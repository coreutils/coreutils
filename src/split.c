/* split.c -- split a file into pieces.
   Copyright (C) 1988-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* By tege@sics.se, with rms.

   TODO:
   * support -p REGEX as in BSD's split.
   * support --suppress-matched as in csplit.  */
#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <spawn.h>

#include "system.h"
#include "alignalloc.h"
#include "assure.h"
#include "fadvise.h"
#include "fd-reopen.h"
#include "fcntl--.h"
#include "full-write.h"
#include "ioblksize.h"
#include "quote.h"
#include "sig2str.h"
#include "sys-limits.h"
#include "temp-stream.h"
#include "xbinary-io.h"
#include "xdectoint.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "split"

#define AUTHORS \
  proper_name_lite ("Torbjorn Granlund", "Torbj\303\266rn Granlund"), \
  proper_name ("Richard M. Stallman")

/* Shell command to filter through, instead of creating files.  */
static char const *filter_command;

/* Process ID of the filter.  */
static pid_t filter_pid;

/* Array of open pipes.  */
static int *open_pipes;
static idx_t open_pipes_alloc;
static int n_open_pipes;

/* Whether SIGPIPE has the default action, when --filter is used.  */
static bool default_SIGPIPE;

/* Base name of output files.  */
static char const *outbase;

/* Name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Generate new suffix when suffixes are exhausted.  */
static bool suffix_auto = true;

/* Length of OUTFILE's suffix.  */
static idx_t suffix_length;

/* Alphabet of characters to use in suffix.  */
static char const *suffix_alphabet = "abcdefghijklmnopqrstuvwxyz";

/* Numerical suffix start value.  */
static char const *numeric_suffix_start;

/* Additional suffix to append to output file names.  */
static char const *additional_suffix;

/* Name of input file.  May be "-".  */
static char const *infile;

/* stat buf for input file.  */
static struct stat in_stat_buf;

/* Descriptor on which output file is open.  */
static int output_desc = -1;

/* If true, print a diagnostic on standard error just before each
   output file is opened. */
static bool verbose;

/* If true, don't generate zero length output files. */
static bool elide_empty_files;

/* If true, in round robin mode, immediately copy
   input to output, which is much slower, so disabled by default.  */
static bool unbuffered;

/* The character marking end of line.  Defaults to \n below.  */
static int eolchar = -1;

/* The split mode to use.  */
enum Split_type
{
  type_undef, type_bytes, type_byteslines, type_lines, type_digits,
  type_chunk_bytes, type_chunk_lines, type_rr
};

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  VERBOSE_OPTION = CHAR_MAX + 1,
  FILTER_OPTION,
  IO_BLKSIZE_OPTION,
  ADDITIONAL_SUFFIX_OPTION
};

static struct option const longopts[] =
{
  {"bytes", required_argument, nullptr, 'b'},
  {"lines", required_argument, nullptr, 'l'},
  {"line-bytes", required_argument, nullptr, 'C'},
  {"number", required_argument, nullptr, 'n'},
  {"elide-empty-files", no_argument, nullptr, 'e'},
  {"unbuffered", no_argument, nullptr, 'u'},
  {"suffix-length", required_argument, nullptr, 'a'},
  {"additional-suffix", required_argument, nullptr,
   ADDITIONAL_SUFFIX_OPTION},
  {"numeric-suffixes", optional_argument, nullptr, 'd'},
  {"hex-suffixes", optional_argument, nullptr, 'x'},
  {"filter", required_argument, nullptr, FILTER_OPTION},
  {"verbose", no_argument, nullptr, VERBOSE_OPTION},
  {"separator", required_argument, nullptr, 't'},
  {"-io-blksize", required_argument, nullptr,
   IO_BLKSIZE_OPTION}, /* do not document */
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

/* Return true if the errno value, ERR, is ignorable.  */
static inline bool
ignorable (int err)
{
  return filter_command && err == EPIPE;
}

static void
set_suffix_length (intmax_t n_units, enum Split_type split_type)
{
#define DEFAULT_SUFFIX_LENGTH 2

  int suffix_length_needed = 0;

  /* The suffix auto length feature is incompatible with
     a user specified start value as the generated suffixes
     are not all consecutive.  */
  if (numeric_suffix_start)
    suffix_auto = false;

  /* Auto-calculate the suffix length if the number of files is given.  */
  if (split_type == type_chunk_bytes || split_type == type_chunk_lines
      || split_type == type_rr)
    {
      intmax_t n_units_end = n_units - 1;
      if (numeric_suffix_start)
        {
          intmax_t n_start;
          strtol_error e = xstrtoimax (numeric_suffix_start, nullptr, 10,
                                       &n_start, "");
          if (e == LONGINT_OK && n_start < n_units)
            {
              /* Restrict auto adjustment so we don't keep
                 incrementing a suffix size arbitrarily,
                 as that would break sort order for files
                 generated from multiple split runs.  */
              if (ckd_add (&n_units_end, n_units_end, n_start))
                n_units_end = INTMAX_MAX;
            }

        }
      idx_t alphabet_len = strlen (suffix_alphabet);
      do
        suffix_length_needed++;
      while (n_units_end /= alphabet_len);

      suffix_auto = false;
    }

  if (suffix_length)            /* set by user */
    {
      if (suffix_length < suffix_length_needed)
        error (EXIT_FAILURE, 0,
               _("the suffix length needs to be at least %d"),
               suffix_length_needed);
      suffix_auto = false;
      return;
    }
  else
    suffix_length = MAX (DEFAULT_SUFFIX_LENGTH, suffix_length_needed);
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE [PREFIX]]\n\
"),
              program_name);
      fputs (_("\
Output pieces of FILE to PREFIXaa, PREFIXab, ...;\n\
default size is 1000 lines, and default PREFIX is 'x'.\n\
"), stdout);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

      fprintf (stdout, _("\
  -a, --suffix-length=N   generate suffixes of length N (default %d)\n\
      --additional-suffix=SUFFIX  append an additional SUFFIX to file names\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of records per output file\n\
  -d                      use numeric suffixes starting at 0, not alphabetic\n\
      --numeric-suffixes[=FROM]  same as -d, but allow setting the start value\
\n\
  -x                      use hex suffixes starting at 0, not alphabetic\n\
      --hex-suffixes[=FROM]  same as -x, but allow setting the start value\n\
  -e, --elide-empty-files  do not generate empty output files with '-n'\n\
      --filter=COMMAND    write to shell COMMAND; file name is $FILE\n\
  -l, --lines=NUMBER      put NUMBER lines/records per output file\n\
  -n, --number=CHUNKS     generate CHUNKS output files; see explanation below\n\
  -t, --separator=SEP     use SEP instead of newline as the record separator;\n\
                            '\\0' (zero) specifies the NUL character\n\
  -u, --unbuffered        immediately copy input to output with '-n r/...'\n\
"), DEFAULT_SUFFIX_LENGTH);
      fputs (_("\
      --verbose           print a diagnostic just before each\n\
                            output file is opened\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_size_note ();
      fputs (_("\n\
CHUNKS may be:\n\
  N       split into N files based on size of input\n\
  K/N     output Kth of N to standard output\n\
  l/N     split into N files without splitting lines/records\n\
  l/K/N   output Kth of N to standard output without splitting lines/records\n\
  r/N     like 'l' but use round robin distribution\n\
  r/K/N   likewise but only output Kth of N to standard output\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Copy the data in FD to a temporary file, then make that file FD.
   Use BUF, of size BUFSIZE, to copy.  Return the number of
   bytes copied, or -1 (setting errno) on error.  */
static off_t
copy_to_tmpfile (int fd, char *buf, idx_t bufsize)
{
  FILE *tmp;
  if (!temp_stream (&tmp, nullptr))
    return -1;
  off_t copied = 0;
  off_t r;

  while (0 < (r = read (fd, buf, bufsize)))
    {
      if (fwrite (buf, 1, r, tmp) != r)
        return -1;
      if (ckd_add (&copied, copied, r))
        {
          errno = EOVERFLOW;
          return -1;
        }
    }

  if (r < 0)
    return r;
  r = dup2 (fileno (tmp), fd);
  if (r < 0)
    return r;
  if (fclose (tmp) < 0)
    return -1;
  return copied;
}

/* Return the number of bytes that can be read from FD with status ST.
   Store up to the first BUFSIZE bytes of the file's data into BUF,
   and advance the file position by the number of bytes read.  On
   input error, set errno and return -1.  */

static off_t
input_file_size (int fd, struct stat const *st, char *buf, idx_t bufsize)
{
  off_t size = 0;
  do
    {
      ssize_t n_read = read (fd, buf + size, bufsize - size);
      if (n_read <= 0)
        return n_read < 0 ? n_read : size;
      size += n_read;
    }
  while (size < bufsize);

  off_t cur, end;
  if ((usable_st_size (st) && st->st_size < size)
      || (cur = lseek (fd, 0, SEEK_CUR)) < 0
      || cur < size /* E.g., /dev/zero on GNU/Linux.  */
      || (end = lseek (fd, 0, SEEK_END)) < 0)
    {
      char *tmpbuf = xmalloc (bufsize);
      end = copy_to_tmpfile (fd, tmpbuf, bufsize);
      free (tmpbuf);
      if (end < 0)
        return end;
      cur = 0;
    }

  if (end == OFF_T_MAX /* E.g., /dev/zero on GNU/Hurd.  */
      || (cur < end && ckd_add (&size, size, end - cur)))
    {
      errno = EOVERFLOW;
      return -1;
    }

  if (cur < end)
    {
      off_t r = lseek (fd, cur, SEEK_SET);
      if (r < 0)
        return r;
    }

  return size;
}

/* Compute the next sequential output file name and store it into the
   string 'outfile'.  */

static void
next_file_name (void)
{
  /* Index in suffix_alphabet of each character in the suffix.  */
  static idx_t *sufindex;
  static idx_t outbase_length;
  static idx_t outfile_length;
  static idx_t addsuf_length;

  if (! outfile)
    {
      bool overflow, widen;

new_name:
      widen = !! outfile_length;

      if (! widen)
        {
          /* Allocate and initialize the first file name.  */

          outbase_length = strlen (outbase);
          addsuf_length = additional_suffix ? strlen (additional_suffix) : 0;
          overflow = ckd_add (&outfile_length, outbase_length + addsuf_length,
                              suffix_length);
        }
      else
        {
          /* Reallocate and initialize a new wider file name.
             We do this by subsuming the unchanging part of
             the generated suffix into the prefix (base), and
             reinitializing the now one longer suffix.  */

          overflow = ckd_add (&outfile_length, outfile_length, 2);
          suffix_length++;
        }

      idx_t outfile_size;
      overflow |= ckd_add (&outfile_size, outfile_length, 1);
      if (overflow)
        xalloc_die ();
      outfile = xirealloc (outfile, outfile_size);

      if (! widen)
        memcpy (outfile, outbase, outbase_length);
      else
        {
          /* Append the last alphabet character to the file name prefix.  */
          outfile[outbase_length] = suffix_alphabet[sufindex[0]];
          outbase_length++;
        }

      outfile_mid = outfile + outbase_length;
      memset (outfile_mid, suffix_alphabet[0], suffix_length);
      if (additional_suffix)
        memcpy (outfile_mid + suffix_length, additional_suffix, addsuf_length);
      outfile[outfile_length] = 0;

      free (sufindex);
      sufindex = xicalloc (suffix_length, sizeof *sufindex);

      if (numeric_suffix_start)
        {
          affirm (! widen);

          /* Update the output file name.  */
          idx_t i = strlen (numeric_suffix_start);
          memcpy (outfile_mid + suffix_length - i, numeric_suffix_start, i);

          /* Update the suffix index.  */
          idx_t *sufindex_end = sufindex + suffix_length;
          while (i-- != 0)
            *--sufindex_end = numeric_suffix_start[i] - '0';
        }

#if ! _POSIX_NO_TRUNC && HAVE_PATHCONF && defined _PC_NAME_MAX
      /* POSIX requires that if the output file name is too long for
         its directory, 'split' must fail without creating any files.
         This must be checked for explicitly on operating systems that
         silently truncate file names.  */
      {
        char *dir = dir_name (outfile);
        long name_max = pathconf (dir, _PC_NAME_MAX);
        if (0 <= name_max && name_max < base_len (last_component (outfile)))
          error (EXIT_FAILURE, ENAMETOOLONG, "%s", quotef (outfile));
        free (dir);
      }
#endif
    }
  else
    {
      /* Increment the suffix in place, if possible.  */

      idx_t i = suffix_length;
      while (i-- != 0)
        {
          sufindex[i]++;
          if (suffix_auto && i == 0 && ! suffix_alphabet[sufindex[0] + 1])
            goto new_name;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
          if (outfile_mid[i])
            return;
          sufindex[i] = 0;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
        }
      error (EXIT_FAILURE, 0, _("output file suffixes exhausted"));
    }
}

/* Create or truncate a file.  */

static int
create (char const *name)
{
  if (!filter_command)
    {
      if (verbose)
        fprintf (stdout, _("creating file %s\n"), quoteaf (name));

      int oflags = O_WRONLY | O_CREAT | O_BINARY;
      int fd = open (name, oflags | O_EXCL, MODE_RW_UGO);
      if (0 <= fd || errno != EEXIST)
        return fd;
      fd = open (name, oflags, MODE_RW_UGO);
      if (fd < 0)
        return fd;
      struct stat out_stat_buf;
      if (fstat (fd, &out_stat_buf) != 0)
        error (EXIT_FAILURE, errno, _("failed to stat %s"), quoteaf (name));
      if (psame_inode (&in_stat_buf, &out_stat_buf))
        error (EXIT_FAILURE, 0, _("%s would overwrite input; aborting"),
               quoteaf (name));
      if (ftruncate (fd, 0) < 0
          && (S_ISREG (out_stat_buf.st_mode) || S_TYPEISSHM (&out_stat_buf)))
        error (EXIT_FAILURE, errno, _("%s: error truncating"), quotef (name));

      return fd;
    }
  else
    {
      if (setenv ("FILE", name, 1) != 0)
        error (EXIT_FAILURE, errno,
               _("failed to set FILE environment variable"));
      if (verbose)
        fprintf (stdout, _("executing with FILE=%s\n"), quotef (name));

      int result;
      int fd_pair[2];
      pid_t child_pid;

      posix_spawnattr_t attr;
      posix_spawn_file_actions_t actions;

      sigset_t set;
      sigemptyset (&set);
      if (default_SIGPIPE)
        sigaddset (&set, SIGPIPE);

      if (   (result = posix_spawnattr_init (&attr))
          || (result = posix_spawnattr_setflags (&attr,
                                                 (POSIX_SPAWN_USEVFORK
                                                  | POSIX_SPAWN_SETSIGDEF)))
          || (result = posix_spawnattr_setsigdefault (&attr, &set))
          || (result = posix_spawn_file_actions_init (&actions))
         )
        error (EXIT_FAILURE, result, _("posix_spawn initialization failed"));

      if (pipe (fd_pair) != 0)
        error (EXIT_FAILURE, errno, _("failed to create pipe"));

      /* We have to close any pipes that were opened during an
         earlier call, otherwise this process will be holding a
         write-pipe that will prevent the earlier process from
         reading an EOF on the corresponding read-pipe.  */
      for (int i = 0; i < n_open_pipes; ++i)
        if ((result = posix_spawn_file_actions_addclose (&actions,
                                                         open_pipes[i])))
          break;

      if (   result
          || (result = posix_spawn_file_actions_addclose (&actions, fd_pair[1]))
          || (fd_pair[0] != STDIN_FILENO
              && (   (result = posix_spawn_file_actions_adddup2 (&actions,
                                                                 fd_pair[0],
                                                                 STDIN_FILENO))
                  || (result = posix_spawn_file_actions_addclose (&actions,
                                                                  fd_pair[0]))))
         )
        error (EXIT_FAILURE, result, _("posix_spawn setup failed"));


      char const *shell_prog = getenv ("SHELL");
      if (shell_prog == nullptr)
        shell_prog = "/bin/sh";
      char const *const argv[] = { last_component (shell_prog), "-c",
                                   filter_command, nullptr };

      result = posix_spawn (&child_pid, shell_prog, &actions, &attr,
                            (char * const *) argv, environ);
      if (result != 0)
        error (EXIT_FAILURE, errno, _("failed to run command: \"%s -c %s\""),
               shell_prog, filter_command);

      posix_spawnattr_destroy (&attr);
      posix_spawn_file_actions_destroy (&actions);

      if (close (fd_pair[0]) != 0)
        error (EXIT_FAILURE, errno, _("failed to close input pipe"));
      filter_pid = child_pid;
      if (n_open_pipes == open_pipes_alloc)
        open_pipes = xpalloc (open_pipes, &open_pipes_alloc, 1,
                              MIN (INT_MAX, IDX_MAX), sizeof *open_pipes);
      open_pipes[n_open_pipes++] = fd_pair[1];
      return fd_pair[1];
    }
}

/* Close the output file, and do any associated cleanup.
   If FP and FD are both specified, they refer to the same open file;
   in this case FP is closed, but FD is still used in cleanup.  */
static void
closeout (FILE *fp, int fd, pid_t pid, char const *name)
{
  if (fp != nullptr && fclose (fp) != 0 && ! ignorable (errno))
    error (EXIT_FAILURE, errno, "%s", quotef (name));
  if (fd >= 0)
    {
      if (fp == nullptr && close (fd) < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (name));
      int j;
      for (j = 0; j < n_open_pipes; ++j)
        {
          if (open_pipes[j] == fd)
            {
              open_pipes[j] = open_pipes[--n_open_pipes];
              break;
            }
        }
    }
  if (pid > 0)
    {
      int wstatus;
      if (waitpid (pid, &wstatus, 0) < 0)
        error (EXIT_FAILURE, errno, _("waiting for child process"));
      else if (WIFSIGNALED (wstatus))
        {
          int sig = WTERMSIG (wstatus);
          if (sig != SIGPIPE)
            {
              char signame[MAX (SIG2STR_MAX, INT_BUFSIZE_BOUND (int))];
              if (sig2str (sig, signame) != 0)
                sprintf (signame, "%d", sig);
              error (sig + 128, 0,
                     _("with FILE=%s, signal %s from command: %s"),
                     quotef (name), signame, filter_command);
            }
        }
      else if (WIFEXITED (wstatus))
        {
          int ex = WEXITSTATUS (wstatus);
          if (ex != 0)
            error (ex, 0, _("with FILE=%s, exit %d from command: %s"),
                   quotef (name), ex, filter_command);
        }
      else
        {
          /* shouldn't happen.  */
          error (EXIT_FAILURE, 0,
                 _("unknown status from command (0x%X)"), wstatus + 0u);
        }
    }
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is true, open the next output file.
   Otherwise add to the same output file already in use.
   Return true if successful.  */

static bool
cwrite (bool new_file_flag, char const *bp, idx_t bytes)
{
  if (new_file_flag)
    {
      if (!bp && bytes == 0 && elide_empty_files)
        return true;
      closeout (nullptr, output_desc, filter_pid, outfile);
      next_file_name ();
      output_desc = create (outfile);
      if (output_desc < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (outfile));
    }

  if (full_write (output_desc, bp, bytes) == bytes)
    return true;
  else
    {
      if (! ignorable (errno))
        error (EXIT_FAILURE, errno, "%s", quotef (outfile));
      return false;
    }
}

/* Split into pieces of exactly N_BYTES bytes.
   However, the first REM_BYTES pieces should be 1 byte longer.
   Use buffer BUF, whose size is BUFSIZE.
   If INITIAL_READ is nonnegative,
   BUF contains the first INITIAL_READ input bytes.  */

static void
bytes_split (intmax_t n_bytes, intmax_t rem_bytes,
             char *buf, idx_t bufsize, ssize_t initial_read,
             intmax_t max_files)
{
  bool new_file_flag = true;
  bool filter_ok = true;
  intmax_t opened = 0;
  intmax_t to_write = n_bytes + (0 < rem_bytes);
  bool eof = ! to_write;

  while (! eof)
    {
      ssize_t n_read;
      if (0 <= initial_read)
        {
          n_read = initial_read;
          initial_read = -1;
          eof = n_read < bufsize;
        }
      else
        {
          if (! filter_ok
              && 0 <= lseek (STDIN_FILENO, to_write, SEEK_CUR))
            {
              to_write = n_bytes + (opened + 1 < rem_bytes);
              new_file_flag = true;
            }

          n_read = read (STDIN_FILENO, buf, bufsize);
          if (n_read < 0)
            error (EXIT_FAILURE, errno, "%s", quotef (infile));
          eof = n_read == 0;
        }
      char *bp_out = buf;
      while (0 < to_write && to_write <= n_read)
        {
          if (filter_ok || new_file_flag)
            filter_ok = cwrite (new_file_flag, bp_out, to_write);
          opened += new_file_flag;
          new_file_flag = !max_files || (opened < max_files);
          if (! filter_ok && ! new_file_flag)
            {
              /* If filters no longer accepting input, stop reading.  */
              n_read = 0;
              eof = true;
              break;
            }
          bp_out += to_write;
          n_read -= to_write;
          to_write = n_bytes + (opened < rem_bytes);
        }
      if (0 < n_read)
        {
          if (filter_ok || new_file_flag)
            filter_ok = cwrite (new_file_flag, bp_out, n_read);
          opened += new_file_flag;
          new_file_flag = false;
          if (! filter_ok && opened == max_files)
            {
              /* If filters no longer accepting input, stop reading.  */
              break;
            }
          to_write -= n_read;
        }
    }

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  while (opened++ < max_files)
    cwrite (true, nullptr, 0);
}

/* Split into pieces of exactly N_LINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (intmax_t n_lines, char *buf, idx_t bufsize)
{
  ssize_t n_read;
  char *bp, *bp_out, *eob;
  bool new_file_flag = true;
  intmax_t n = 0;

  do
    {
      n_read = read (STDIN_FILENO, buf, bufsize);
      if (n_read < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
      bp = bp_out = buf;
      eob = bp + n_read;
      *eob = eolchar;
      while (true)
        {
          bp = rawmemchr (bp, eolchar);
          if (bp == eob)
            {
              if (eob != bp_out) /* do not write 0 bytes! */
                {
                  idx_t len = eob - bp_out;
                  cwrite (new_file_flag, bp_out, len);
                  new_file_flag = false;
                }
              break;
            }

          ++bp;
          if (++n >= n_lines)
            {
              cwrite (new_file_flag, bp_out, bp - bp_out);
              bp_out = bp;
              new_file_flag = true;
              n = 0;
            }
        }
    }
  while (n_read);
}

/* Split into pieces that are as large as possible while still not more
   than N_BYTES bytes, and are split on line boundaries except
   where lines longer than N_BYTES bytes occur. */

static void
line_bytes_split (intmax_t n_bytes, char *buf, idx_t bufsize)
{
  ssize_t n_read;
  intmax_t n_out = 0;      /* for each split.  */
  idx_t n_hold = 0;
  char *hold = nullptr;        /* for lines > bufsize.  */
  idx_t hold_size = 0;
  bool split_line = false;  /* Whether a \n was output in a split.  */

  do
    {
      n_read = read (STDIN_FILENO, buf, bufsize);
      if (n_read < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
      idx_t n_left = n_read;
      char *sob = buf;
      while (n_left)
        {
          idx_t split_rest = 0;
          char *eoc = nullptr;
          char *eol;

          /* Determine End Of Chunk and/or End of Line,
             which are used below to select what to write or buffer.  */
          if (n_bytes - n_out - n_hold <= n_left)
            {
              /* Have enough for split.  */
              split_rest = n_bytes - n_out - n_hold;
              eoc = sob + split_rest - 1;
              eol = memrchr (sob, eolchar, split_rest);
            }
          else
            eol = memrchr (sob, eolchar, n_left);

          /* Output hold space if possible.  */
          if (n_hold && !(!eol && n_out))
            {
              cwrite (n_out == 0, hold, n_hold);
              n_out += n_hold;
              n_hold = 0;
            }

          /* Output to eol if present.  */
          if (eol)
            {
              split_line = true;
              idx_t n_write = eol - sob + 1;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Output to eoc or eob if possible.  */
          if (n_left && !split_line)
            {
              idx_t n_write = eoc ? split_rest : n_left;
              cwrite (n_out == 0, sob, n_write);
              n_out += n_write;
              n_left -= n_write;
              sob += n_write;
              if (eoc)
                split_rest -= n_write;
            }

          /* Update hold if needed.  */
          if ((eoc && split_rest) || (!eoc && n_left))
            {
              idx_t n_buf = eoc ? split_rest : n_left;
              if (hold_size - n_hold < n_buf)
                hold = xpalloc (hold, &hold_size, n_buf - (hold_size - n_hold),
                                -1, sizeof *hold);
              memcpy (hold + n_hold, sob, n_buf);
              n_hold += n_buf;
              n_left -= n_buf;
              sob += n_buf;
            }

          /* Reset for new split.  */
          if (eoc)
            {
              n_out = 0;
              split_line = false;
            }
        }
    }
  while (n_read);

  /* Handle no eol at end of file.  */
  if (n_hold)
    cwrite (n_out == 0, hold, n_hold);

  free (hold);
}

/* -n l/[K/]N: Write lines to files of approximately file size / N.
   The file is partitioned into file size / N sized portions, with the
   last assigned any excess.  If a line _starts_ within a partition
   it is written completely to the corresponding file.  Since lines
   are not split even if they overlap a partition, the files written
   can be larger or smaller than the partition size, and even empty
   if a line is so long as to completely overlap the partition.  */

static void
lines_chunk_split (intmax_t k, intmax_t n, char *buf, idx_t bufsize,
                   ssize_t initial_read, off_t file_size)
{
  affirm (n && k <= n);

  intmax_t rem_bytes = file_size % n;
  off_t chunk_size = file_size / n;
  intmax_t chunk_no = 1;
  off_t chunk_end = chunk_size + (0 < rem_bytes);
  off_t n_written = 0;
  bool new_file_flag = true;
  bool chunk_truncated = false;

  if (k > 1 && 0 < file_size)
    {
      /* Start reading 1 byte before kth chunk of file.  */
      off_t start = (k - 1) * chunk_size + MIN (k - 1, rem_bytes) - 1;
      if (start < initial_read)
        {
          memmove (buf, buf + start, initial_read - start);
          initial_read -= start;
        }
      else
        {
          if (initial_read < start
              && lseek (STDIN_FILENO, start - initial_read, SEEK_CUR) < 0)
            error (EXIT_FAILURE, errno, "%s", quotef (infile));
          initial_read = -1;
        }
      n_written = start;
      chunk_no = k - 1;
      chunk_end = start + 1;
    }

  while (n_written < file_size)
    {
      char *bp = buf, *eob;
      ssize_t n_read;
      if (0 <= initial_read)
        {
          n_read = initial_read;
          initial_read = -1;
        }
      else
        {
          n_read = read (STDIN_FILENO, buf,
                         MIN (bufsize, file_size - n_written));
          if (n_read < 0)
            error (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      chunk_truncated = false;
      eob = buf + n_read;

      while (bp != eob)
        {
          idx_t to_write;
          bool next = false;

          /* Begin looking for '\n' at last byte of chunk.  */
          off_t skip = MIN (n_read, MAX (0, chunk_end - 1 - n_written));
          char *bp_out = memchr (bp + skip, eolchar, n_read - skip);
          if (bp_out)
            {
              bp_out++;
              next = true;
            }
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k == chunk_no)
            {
              /* We don't use the stdout buffer here since we're writing
                 large chunks from an existing file, so it's more efficient
                 to write out directly.  */
              if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                write_error ();
            }
          else if (! k)
            cwrite (new_file_flag, bp, to_write);
          n_written += to_write;
          bp += to_write;
          n_read -= to_write;
          new_file_flag = next;

          /* A line could have been so long that it skipped
             entire chunks. So create empty files in that case.  */
          while (next || chunk_end <= n_written)
            {
              if (!next && bp == eob)
                {
                  /* replenish buf, before going to next chunk.  */
                  chunk_truncated = true;
                  break;
                }
              if (k == chunk_no)
                return;
              chunk_end += chunk_size + (chunk_no < rem_bytes);
              chunk_no++;
              if (chunk_end <= n_written)
                {
                  if (! k)
                    cwrite (true, nullptr, 0);
                }
              else
                next = false;
            }
        }
    }

  if (chunk_truncated)
    chunk_no++;

  /* Ensure NUMBER files are created, which truncates
     any existing files or notifies any consumers on fifos.
     FIXME: Should we do this before EXIT_FAILURE?  */
  if (!k)
    while (chunk_no++ <= n)
      cwrite (true, nullptr, 0);
}

/* -n K/N: Extract Kth of N chunks.  */

static void
bytes_chunk_extract (intmax_t k, intmax_t n, char *buf, idx_t bufsize,
                     ssize_t initial_read, off_t file_size)
{
  off_t start;
  off_t end;

  assert (0 < k && k <= n);

  start = (k - 1) * (file_size / n) + MIN (k - 1, file_size % n);
  end = k == n ? file_size : k * (file_size / n) + MIN (k, file_size % n);

  if (start < initial_read)
    {
      memmove (buf, buf + start, initial_read - start);
      initial_read -= start;
    }
  else
    {
      if (initial_read < start
          && lseek (STDIN_FILENO, start - initial_read, SEEK_CUR) < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
      initial_read = -1;
    }

  while (start < end)
    {
      ssize_t n_read;
      if (0 <= initial_read)
        {
          n_read = initial_read;
          initial_read = -1;
        }
      else
        {
          n_read = read (STDIN_FILENO, buf, bufsize);
          if (n_read < 0)
            error (EXIT_FAILURE, errno, "%s", quotef (infile));
        }
      if (n_read == 0)
        break; /* eof.  */
      n_read = MIN (n_read, end - start);
      if (full_write (STDOUT_FILENO, buf, n_read) != n_read
          && ! ignorable (errno))
        error (EXIT_FAILURE, errno, "%s", quotef ("-"));
      start += n_read;
    }
}

typedef struct of_info
{
  char *of_name;
  int ofd;
  FILE *ofile;
  pid_t opid;
} of_t;

enum
{
  OFD_NEW = -1,
  OFD_APPEND = -2
};

/* Rotate file descriptors when we're writing to more output files than we
   have available file descriptors.
   Return whether we came under file resource pressure.
   If so, it's probably best to close each file when finished with it.  */

static bool
ofile_open (of_t *files, idx_t i_check, idx_t nfiles)
{
  bool file_limit = false;

  if (files[i_check].ofd <= OFD_NEW)
    {
      int fd;
      idx_t i_reopen = i_check ? i_check - 1 : nfiles - 1;

      /* Another process could have opened a file in between the calls to
         close and open, so we should keep trying until open succeeds or
         we've closed all of our files.  */
      while (true)
        {
          if (files[i_check].ofd == OFD_NEW)
            fd = create (files[i_check].of_name);
          else /* OFD_APPEND  */
            {
              /* Attempt to append to previously opened file.
                 We use O_NONBLOCK to support writing to fifos,
                 where the other end has closed because of our
                 previous close.  In that case we'll immediately
                 get an error, rather than waiting indefinitely.
                 In specialized cases the consumer can keep reading
                 from the fifo, terminating on conditions in the data
                 itself, or perhaps never in the case of 'tail -f'.
                 I.e., for fifos it is valid to attempt this reopen.

                 We don't handle the filter_command case here, as create()
                 will exit if there are not enough files in that case.
                 I.e., we don't support restarting filters, as that would
                 put too much burden on users specifying --filter commands.  */
              fd = open (files[i_check].of_name,
                         O_WRONLY | O_BINARY | O_APPEND | O_NONBLOCK);
            }

          if (0 <= fd)
            break;

          if (!(errno == EMFILE || errno == ENFILE))
            error (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));

          file_limit = true;

          /* Search backwards for an open file to close.  */
          while (files[i_reopen].ofd < 0)
            {
              i_reopen = i_reopen ? i_reopen - 1 : nfiles - 1;
              /* No more open files to close, exit with E[NM]FILE.  */
              if (i_reopen == i_check)
                error (EXIT_FAILURE, errno, "%s",
                       quotef (files[i_check].of_name));
            }

          if (fclose (files[i_reopen].ofile) != 0)
            error (EXIT_FAILURE, errno, "%s", quotef (files[i_reopen].of_name));
          files[i_reopen].ofile = nullptr;
          files[i_reopen].ofd = OFD_APPEND;
        }

      files[i_check].ofd = fd;
      FILE *ofile = fdopen (fd, "a");
      if (!ofile)
        error (EXIT_FAILURE, errno, "%s", quotef (files[i_check].of_name));
      files[i_check].ofile = ofile;
      files[i_check].opid = filter_pid;
      filter_pid = 0;
    }

  return file_limit;
}

/* -n r/[K/]N: Divide file into N chunks in round robin fashion.
   Use BUF of size BUFSIZE for the buffer, and if allocating storage
   put its address into *FILESP to pacify -fsanitize=leak.
   When K == 0, we try to keep the files open in parallel.
   If we run out of file resources, then we revert
   to opening and closing each file for each line.  */

static void
lines_rr (intmax_t k, intmax_t n, char *buf, idx_t bufsize, of_t **filesp)
{
  bool wrapped = false;
  bool wrote = false;
  bool file_limit;
  idx_t i_file;
  of_t *files IF_LINT (= nullptr);
  intmax_t line_no;

  if (k)
    line_no = 1;
  else
    {
      if (IDX_MAX < n)
        xalloc_die ();
      files = *filesp = xinmalloc (n, sizeof *files);

      /* Generate output file names. */
      for (i_file = 0; i_file < n; i_file++)
        {
          next_file_name ();
          files[i_file].of_name = xstrdup (outfile);
          files[i_file].ofd = OFD_NEW;
          files[i_file].ofile = nullptr;
          files[i_file].opid = 0;
        }
      i_file = 0;
      file_limit = false;
    }

  while (true)
    {
      char *bp = buf, *eob;
      ssize_t n_read = read (STDIN_FILENO, buf, bufsize);
      if (n_read < 0)
        error (EXIT_FAILURE, errno, "%s", quotef (infile));
      else if (n_read == 0)
        break; /* eof.  */
      eob = buf + n_read;

      while (bp != eob)
        {
          idx_t to_write;
          bool next = false;

          /* Find end of line. */
          char *bp_out = memchr (bp, eolchar, eob - bp);
          if (bp_out)
            {
              bp_out++;
              next = true;
            }
          else
            bp_out = eob;
          to_write = bp_out - bp;

          if (k)
            {
              if (line_no == k && unbuffered)
                {
                  if (full_write (STDOUT_FILENO, bp, to_write) != to_write)
                    write_error ();
                }
              else if (line_no == k && fwrite (bp, to_write, 1, stdout) != 1)
                {
                  write_error ();
                }
              if (next)
                line_no = (line_no == n) ? 1 : line_no + 1;
            }
          else
            {
              /* Secure file descriptor. */
              file_limit |= ofile_open (files, i_file, n);
              if (unbuffered)
                {
                  /* Note writing to fd, rather than flushing the FILE gives
                     an 8% performance benefit, due to reduced data copying.  */
                  if (full_write (files[i_file].ofd, bp, to_write) != to_write
                      && ! ignorable (errno))
                    error (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                }
              else if (fwrite (bp, to_write, 1, files[i_file].ofile) != 1
                       && ! ignorable (errno))
                error (EXIT_FAILURE, errno, "%s",
                       quotef (files[i_file].of_name));

              if (! ignorable (errno))
                wrote = true;

              if (file_limit)
                {
                  if (fclose (files[i_file].ofile) != 0)
                    error (EXIT_FAILURE, errno, "%s",
                           quotef (files[i_file].of_name));
                  files[i_file].ofile = nullptr;
                  files[i_file].ofd = OFD_APPEND;
                }
              if (next && ++i_file == n)
                {
                  wrapped = true;
                  /* If no filters are accepting input, stop reading.  */
                  if (! wrote)
                    goto no_filters;
                  wrote = false;
                  i_file = 0;
                }
            }

          bp = bp_out;
        }
    }

no_filters:
  /* Ensure all files created, so that any existing files are truncated,
     and to signal any waiting fifo consumers.
     Also, close any open file descriptors.
     FIXME: Should we do this before EXIT_FAILURE?  */
  if (!k)
    {
      idx_t ceiling = wrapped ? n : i_file;
      for (i_file = 0; i_file < n; i_file++)
        {
          if (i_file >= ceiling && !elide_empty_files)
            file_limit |= ofile_open (files, i_file, n);
          if (files[i_file].ofd >= 0)
            closeout (files[i_file].ofile, files[i_file].ofd,
                      files[i_file].opid, files[i_file].of_name);
          files[i_file].ofd = OFD_APPEND;
        }
    }
}

#define FAIL_ONLY_ONE_WAY()					\
  do								\
    {								\
      error (0, 0, _("cannot split in more than one way"));	\
      usage (EXIT_FAILURE);					\
    }								\
  while (0)

/* Report a string-to-integer conversion failure MSGID with ARG.  */

static _Noreturn void
strtoint_die (char const *msgid, char const *arg)
{
  error (EXIT_FAILURE, errno == EINVAL ? 0 : errno, "%s: %s",
         gettext (msgid), quote (arg));
}

/* Use OVERFLOW_OK when it is OK to ignore LONGINT_OVERFLOW errors, since the
   extreme value will do the right thing anyway on any practical platform.  */
#define OVERFLOW_OK LONGINT_OVERFLOW

/* Parse ARG for number of bytes or lines.  The number can be followed
   by MULTIPLIERS, and the resulting value must be positive.
   If the number cannot be parsed, diagnose with MSG.
   Return the number parsed, or an INTMAX_MAX on overflow.  */

static intmax_t
parse_n_units (char const *arg, char const *multipliers, char const *msgid)
{
  intmax_t n;
  if (OVERFLOW_OK < xstrtoimax (arg, nullptr, 10, &n, multipliers) || n < 1)
    strtoint_die (msgid, arg);
  return n;
}

/* Parse K/N syntax of chunk options.  */

static void
parse_chunk (intmax_t *k_units, intmax_t *n_units, char const *arg)
{
  char *argend;
  strtol_error e = xstrtoimax (arg, &argend, 10, n_units, "");
  if (e == LONGINT_INVALID_SUFFIX_CHAR && *argend == '/')
    {
      *k_units = *n_units;
      *n_units = parse_n_units (argend + 1, "",
                                N_("invalid number of chunks"));
      if (! (0 < *k_units && *k_units <= *n_units))
        error (EXIT_FAILURE, 0, "%s: %s", _("invalid chunk number"),
               quote_mem (arg, argend - arg));
    }
  else if (! (e <= OVERFLOW_OK && 0 < *n_units))
    strtoint_die (N_("invalid number of chunks"), arg);
}


int
main (int argc, char **argv)
{
  enum Split_type split_type = type_undef;
  idx_t in_blk_size = 0;	/* optimal block size of input file device */
  idx_t page_size = getpagesize ();
  intmax_t k_units = 0;
  intmax_t n_units = 0;

  static char const multipliers[] = "bEGKkMmPQRTYZ0";
  int c;
  int digits_optind = 0;
  off_t file_size = OFF_T_MAX;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Parse command line options.  */

  infile = "-";
  outbase = "x";

  while (true)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;

      c = getopt_long (argc, argv, "0123456789C:a:b:del:n:t:ux",
                       longopts, nullptr);
      if (c == -1)
        break;

      switch (c)
        {
        case 'a':
          suffix_length = xdectoimax (optarg, 0, IDX_MAX,
                                      "", _("invalid suffix length"), 0);
          break;

        case ADDITIONAL_SUFFIX_OPTION:
          {
            int suffix_len = strlen (optarg);
            if (last_component (optarg) != optarg
                || (suffix_len && ISSLASH (optarg[suffix_len - 1])))
              {
                error (0, 0,
                       _("invalid suffix %s, contains directory separator"),
                       quote (optarg));
                usage (EXIT_FAILURE);
              }
          }
          additional_suffix = optarg;
          break;

        case 'b':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_bytes;
          n_units = parse_n_units (optarg, multipliers,
                                   N_("invalid number of bytes"));
          break;

        case 'l':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_lines;
          n_units = parse_n_units (optarg, "", N_("invalid number of lines"));
          break;

        case 'C':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_byteslines;
          n_units = parse_n_units (optarg, multipliers,
                                   N_("invalid number of lines"));
          break;

        case 'n':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          /* skip any whitespace */
          while (isspace (to_uchar (*optarg)))
            optarg++;
          if (STRNCMP_LIT (optarg, "r/") == 0)
            {
              split_type = type_rr;
              optarg += 2;
            }
          else if (STRNCMP_LIT (optarg, "l/") == 0)
            {
              split_type = type_chunk_lines;
              optarg += 2;
            }
          else
            split_type = type_chunk_bytes;
          parse_chunk (&k_units, &n_units, optarg);
          break;

        case 'u':
          unbuffered = true;
          break;

        case 't':
          {
            char neweol = optarg[0];
            if (! neweol)
              error (EXIT_FAILURE, 0, _("empty record separator"));
            if (optarg[1])
              {
                if (streq (optarg, "\\0"))
                  neweol = '\0';
                else
                  {
                    /* Provoke with 'split -txx'.  Complain about
                       "multi-character tab" instead of "multibyte tab", so
                       that the diagnostic's wording does not need to be
                       changed once multibyte characters are supported.  */
                    error (EXIT_FAILURE, 0, _("multi-character separator %s"),
                           quote (optarg));
                  }
              }
            /* Make it explicit we don't support multiple separators.  */
            if (0 <= eolchar && neweol != eolchar)
              {
                error (EXIT_FAILURE, 0,
                       _("multiple separator characters specified"));
              }

            eolchar = neweol;
          }
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (split_type == type_undef)
            {
              split_type = type_digits;
              n_units = 0;
            }
          if (split_type != type_undef && split_type != type_digits)
            FAIL_ONLY_ONE_WAY ();
          if (digits_optind != 0 && digits_optind != this_optind)
            n_units = 0;	/* More than one number given; ignore other. */
          digits_optind = this_optind;
          if (ckd_mul (&n_units, n_units, 10)
              || ckd_add (&n_units, n_units, c - '0'))
            n_units = INTMAX_MAX;
          break;

        case 'd':
        case 'x':
          if (c == 'd')
            suffix_alphabet = "0123456789";
          else
            suffix_alphabet = "0123456789abcdef";
          if (optarg)
            {
              if (strlen (optarg) != strspn (optarg, suffix_alphabet))
                {
                  error (0, 0,
                         (c == 'd') ?
                           _("%s: invalid start value for numerical suffix") :
                           _("%s: invalid start value for hexadecimal suffix"),
                         quote (optarg));
                  usage (EXIT_FAILURE);
                }
              else
                {
                  /* Skip any leading zero.  */
                  while (*optarg == '0' && *(optarg + 1) != '\0')
                    optarg++;
                  numeric_suffix_start = optarg;
                }
            }
          break;

        case 'e':
          elide_empty_files = true;
          break;

        case FILTER_OPTION:
          filter_command = optarg;
          break;

        case IO_BLKSIZE_OPTION:
          in_blk_size = xnumtoumax (optarg, 10, 1,
                                    MIN (SYS_BUFSIZE_MAX,
                                         MIN (IDX_MAX, SIZE_MAX) - 1),
                                    multipliers, _("invalid IO block size"),
                                    0, XTOINT_MIN_RANGE);
          break;

        case VERBOSE_OPTION:
          verbose = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (k_units != 0 && filter_command)
    {
      error (0, 0, _("--filter does not process a chunk extracted to "
                     "standard output"));
      usage (EXIT_FAILURE);
    }

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      n_units = 1000;
    }

  if (n_units == 0)
    {
      error (0, 0, _("invalid number of lines: %s"), quote ("0"));
      usage (EXIT_FAILURE);
    }

  if (eolchar < 0)
    eolchar = '\n';

  set_suffix_length (n_units, split_type);

  /* Get out the filename arguments.  */

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outbase = argv[optind++];

  if (optind < argc)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind]));
      usage (EXIT_FAILURE);
    }

  /* Check that the suffix length is large enough for the numerical
     suffix start value.  */
  if (numeric_suffix_start && strlen (numeric_suffix_start) > suffix_length)
    {
      error (0, 0, _("numerical suffix start value is too large "
                     "for the suffix length"));
      usage (EXIT_FAILURE);
    }

  /* Open the input file.  */
  if (! streq (infile, "-")
      && fd_reopen (STDIN_FILENO, infile, O_RDONLY, 0) < 0)
    error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
           quoteaf (infile));

  /* Binary I/O is safer when byte counts are used.  */
  xset_binary_mode (STDIN_FILENO, O_BINARY);

  /* Advise the kernel of our access pattern.  */
  fdadvise (STDIN_FILENO, 0, 0, FADVISE_SEQUENTIAL);

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (STDIN_FILENO, &in_stat_buf) != 0)
    error (EXIT_FAILURE, errno, "%s", quotef (infile));

  if (in_blk_size == 0)
    {
      in_blk_size = io_blksize (&in_stat_buf);
      if (SYS_BUFSIZE_MAX < in_blk_size)
        in_blk_size = SYS_BUFSIZE_MAX;
    }

  char *buf = xalignalloc (page_size, in_blk_size + 1);
  ssize_t initial_read = -1;

  if (split_type == type_chunk_bytes || split_type == type_chunk_lines)
    {
      file_size = input_file_size (STDIN_FILENO, &in_stat_buf,
                                   buf, in_blk_size);
      if (file_size < 0)
        error (EXIT_FAILURE, errno, _("%s: cannot determine file size"),
               quotef (infile));
      initial_read = MIN (file_size, in_blk_size);
    }

  /* When filtering, closure of one pipe must not terminate the process,
     as there may still be other streams expecting input from us.  */
  if (filter_command)
    default_SIGPIPE = signal (SIGPIPE, SIG_IGN) == SIG_DFL;

  switch (split_type)
    {
    case type_digits:
    case type_lines:
      lines_split (n_units, buf, in_blk_size);
      break;

    case type_bytes:
      bytes_split (n_units, 0, buf, in_blk_size, -1, 0);
      break;

    case type_byteslines:
      line_bytes_split (n_units, buf, in_blk_size);
      break;

    case type_chunk_bytes:
      if (k_units == 0)
        bytes_split (file_size / n_units, file_size % n_units,
                     buf, in_blk_size, initial_read, n_units);
      else
        bytes_chunk_extract (k_units, n_units, buf, in_blk_size, initial_read,
                             file_size);
      break;

    case type_chunk_lines:
      lines_chunk_split (k_units, n_units, buf, in_blk_size, initial_read,
                         file_size);
      break;

    case type_rr:
      /* Note, this is like 'sed -n ${k}~${n}p' when k > 0,
         but the functionality is provided for symmetry.  */
      {
        of_t *files;
        lines_rr (k_units, n_units, buf, in_blk_size, &files);
      }
      break;

    case type_undef: default:
      affirm (false);
    }

  if (close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, "%s", quotef (infile));
  closeout (nullptr, output_desc, filter_pid, outfile);

  main_exit (EXIT_SUCCESS);
}
