/* tail -- output the last part of file(s)
   Copyright (C) 1989-2025 Free Software Foundation, Inc.

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

/* Can display any amount of data, unlike the Unix version, which uses
   a fixed size buffer and therefore can only deliver a limited number
   of lines.

   Original version by Paul Rubin <phr@ocf.berkeley.edu>.
   Extensions by David MacKenzie <djm@gnu.ai.mit.edu>.
   tail -f for multiple files by Ian Lance Taylor <ian@airs.com>.
   inotify back-end by Giuseppe Scrivano <gscrivano@gnu.org>.  */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>

#include "system.h"
#include "argmatch.h"
#include "assure.h"
#include "c-ctype.h"
#include "cl-strtod.h"
#include "dtimespec-bound.h"
#include "fcntl--.h"
#include "iopoll.h"
#include "isapipe.h"
#include "issymlink.h"
#include "posixver.h"
#include "quote.h"
#include "stat-size.h"
#include "stat-time.h"
#include "xbinary-io.h"
#include "xdectoint.h"
#include "xnanosleep.h"

#if HAVE_INOTIFY
# include "hash.h"
# include <poll.h>
# include <sys/inotify.h>
#endif

/* Linux can optimize the handling of local files.  */
#if defined __linux__ || defined __ANDROID__
# include "fs.h"
# include "fs-is-local.h"
# if HAVE_SYS_STATFS_H
#  include <sys/statfs.h>
# elif HAVE_SYS_VFS_H
#  include <sys/vfs.h>
# endif
#endif

/* The official name of this program (e.g., no 'g' prefix).  */
#define PROGRAM_NAME "tail"

#define AUTHORS \
  proper_name ("Paul Rubin"), \
  proper_name ("David MacKenzie"), \
  proper_name ("Ian Lance Taylor"), \
  proper_name ("Jim Meyering")

/* Number of items to tail.  */
#define DEFAULT_N_LINES 10

/* Special values for dump_remainder's N_BYTES parameter.  */
enum { COPY_TO_EOF = -1, COPY_A_BUFFER = -2 };

/* The type of a large counter.  Although it is always nonnegative,
   it is signed to help signed overflow checking catch any bugs.  */
typedef intmax_t count_t;
#define COUNT_MAX INTMAX_MAX

/* FIXME: make Follow_name the default?  */
#define DEFAULT_FOLLOW_MODE Follow_descriptor

enum Follow_mode
{
  /* Follow the name of each file: if the file is renamed, try to reopen
     that name and track the end of the new file if/when it's recreated.
     This is useful for tracking logs that are occasionally rotated.  */
  Follow_name = 1,

  /* Follow each descriptor obtained upon opening a file.
     That means we'll continue to follow the end of a file even after
     it has been renamed or unlinked.  */
  Follow_descriptor = 2
};

/* The types of files for which tail works.  */
#define IS_TAILABLE_FILE_TYPE(Mode) \
  (S_ISREG (Mode) || S_ISFIFO (Mode) || S_ISSOCK (Mode) || S_ISCHR (Mode))

static char const *const follow_mode_string[] =
{
  "descriptor", "name", nullptr
};

static enum Follow_mode const follow_mode_map[] =
{
  Follow_descriptor, Follow_name,
};

struct File_spec
{
  /* The actual file name, or "-" for stdin.  */
  char *name;

  /* The prettified name; the same as NAME unless NAME is "-".  */
  char const *prettyname;

  /* Attributes of the file the last time we checked.  */
  struct timespec mtime;
  dev_t st_dev;
  ino_t st_ino;
  mode_t mode;

  /* If a regular file, the file's read position the last time we
     checked.  For non-regular files, the value is unspecified.  */
  off_t read_pos;

  /* The specified name initially referred to a directory or some other
     type for which tail isn't meaningful.  Unlike for a permission problem
     (tailable, below) once this is set, the name is not checked ever again.  */
  bool ignore;

  /* See the description of fremote.  */
  bool remote;

  /* A file is tailable if it exists, is readable, and is of type
     IS_TAILABLE_FILE_TYPE.  */
  bool tailable;

  /* File descriptor on which the file is open; -1 if it's not open.  */
  int fd;

  /* 0 if the file's last op succeeded, otherwise the op's errno if
     applicable, otherwise -1.  */
  int errnum;

  /* 1 if O_NONBLOCK is clear, 0 if set, -1 if not known.  */
  int blocking;

#if HAVE_INOTIFY
  /* The watch descriptor used by inotify.  */
  int wd;

  /* The parent directory watch descriptor.  It is used only
   * when Follow_name is used.  */
  int parent_wd;

  /* Offset in NAME of the basename part.  */
  idx_t basename_start;
#endif

  /* See description of DEFAULT_MAX_N_... below.  */
  count_t n_unchanged_stats;
};

/* Keep trying to open a file even if it is inaccessible when tail starts
   or if it becomes inaccessible later -- useful only with -f.  */
static bool reopen_inaccessible_files;

/* If true, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static bool count_lines;

/* Whether we follow the name of each file or the file descriptor
   that is initially associated with each name.  */
static enum Follow_mode follow_mode = Follow_descriptor;

/* If true, read from the ends of all specified files until killed.  */
static bool forever;

/* If true, monitor output so we exit if pipe reader terminates.  */
static bool monitor_output;

/* If true, count from start of file instead of end.  */
static bool from_start;

/* If true, print filename headers.  */
static bool print_headers;

/* Character to split lines by. */
static char line_end;

/* When to print the filename banners.  */
enum header_mode
{
  multiple_files, always, never
};

/* When tailing a file by name, if there have been this many consecutive
   iterations for which the file has not changed, then open/fstat
   the file to determine if that file name is still associated with the
   same device/inode-number pair as before.  This option is meaningful only
   when following by name.  --max-unchanged-stats=N  */
enum { DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS = 5 };
static count_t max_n_unchanged_stats_between_opens =
  DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS;

/* The process IDs of the processes to watch (those writing the followed
   files, or perhaps other processes the user cares about).  */
static int nbpids = 0;
static pid_t * pids = nullptr;
static idx_t pids_alloc;

/* Used to determine the buffer size when scanning backwards in a file.  */
static idx_t page_size;

/* True if we have ever read standard input.  */
static bool have_read_stdin;

/* If nonzero, skip the is-regular-file test used to determine whether
   to use the lseek optimization.  Instead, use the more general (and
   more expensive) code unconditionally. Intended solely for testing.  */
static bool presume_input_pipe;

/* If nonzero then don't use inotify even if available.  */
static bool disable_inotify;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  RETRY_OPTION = CHAR_MAX + 1,
  MAX_UNCHANGED_STATS_OPTION,
  PID_OPTION,
  PRESUME_INPUT_PIPE_OPTION,
  LONG_FOLLOW_OPTION,
  DISABLE_INOTIFY_OPTION
};

static struct option const long_options[] =
{
  {"bytes", required_argument, nullptr, 'c'},
  {"follow", optional_argument, nullptr, LONG_FOLLOW_OPTION},
  {"lines", required_argument, nullptr, 'n'},
  {"max-unchanged-stats", required_argument, nullptr,
   MAX_UNCHANGED_STATS_OPTION},
  {"-disable-inotify", no_argument, nullptr,
   DISABLE_INOTIFY_OPTION}, /* do not document */
  {"pid", required_argument, nullptr, PID_OPTION},
  {"-presume-input-pipe", no_argument, nullptr,
   PRESUME_INPUT_PIPE_OPTION}, /* do not document */
  {"quiet", no_argument, nullptr, 'q'},
  {"retry", no_argument, nullptr, RETRY_OPTION},
  {"silent", no_argument, nullptr, 'q'},
  {"sleep-interval", required_argument, nullptr, 's'},
  {"verbose", no_argument, nullptr, 'v'},
  {"zero-terminated", no_argument, nullptr, 'z'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {nullptr, 0, nullptr, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"),
              program_name);
      printf (_("\
Print the last %d lines of each FILE to standard output.\n\
With more than one FILE, precede each with a header giving the file name.\n\
"), DEFAULT_N_LINES);

      emit_stdin_note ();
      emit_mandatory_arg_note ();

     fputs (_("\
  -c, --bytes=[+]NUM       output the last NUM bytes; or use -c +NUM to\n\
                             output starting with byte NUM of each file\n\
"), stdout);
     fputs (_("\
  -f, --follow[={name|descriptor}]\n\
                           output appended data as the file grows;\n\
                             an absent option argument means 'descriptor'\n\
  -F                       same as --follow=name --retry\n\
"), stdout);
     printf (_("\
  -n, --lines=[+]NUM       output the last NUM lines, instead of the last %d;\n\
                             or use -n +NUM to skip NUM-1 lines at the start\n\
"),
             DEFAULT_N_LINES
             );
     printf (_("\
      --max-unchanged-stats=N\n\
                           with --follow=name, reopen a FILE which has not\n\
                             changed size after N (default %d) iterations\n\
                             to see if it has been unlinked or renamed\n\
                             (this is the usual case of rotated log files);\n\
                             with inotify, this option is rarely useful\n\
"),
             DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS
             );
     fputs (_("\
      --pid=PID            with -f, exit after PID no longer exists;\n\
                             can be repeated to watch multiple processes\n\
  -q, --quiet, --silent    never output headers giving file names\n\
      --retry              keep trying to open a file if it is inaccessible\n\
"), stdout);
     fputs (_("\
  -s, --sleep-interval=N   with -f, sleep for approximately N seconds\n\
                             (default 1.0) between iterations;\n\
                             with inotify and --pid=P, check process P at\n\
                             least once every N seconds\n\
  -v, --verbose            always output headers giving file names\n\
"), stdout);
     fputs (_("\
  -z, --zero-terminated    line delimiter is NUL, not newline\n\
"), stdout);
     fputs (HELP_OPTION_DESCRIPTION, stdout);
     fputs (VERSION_OPTION_DESCRIPTION, stdout);
     fputs (_("\
\n\
NUM may have a multiplier suffix:\n\
b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n\
GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y, R, Q.\n\
Binary prefixes can be used, too: KiB=K, MiB=M, and so on.\n\
\n\
"), stdout);
     fputs (_("\
With --follow (-f), tail defaults to following the file descriptor, which\n\
means that even if a tail'ed file is renamed, tail will continue to track\n\
its end.  This default behavior is not desirable when you really want to\n\
track the actual name of the file, not the file descriptor (e.g., log\n\
rotation).  Use --follow=name in that case.  That causes tail to track the\n\
named file in a way that accommodates renaming, removal and creation.\n\
"), stdout);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}

/* Ensure exit, either with SIGPIPE or EXIT_FAILURE status.  */
static void
die_pipe (void)
{
  raise (SIGPIPE);
  exit (EXIT_FAILURE);
}

/* If the output has gone away, then terminate
   as we would if we had written to this output.  */
static void
check_output_alive (void)
{
  if (! monitor_output)
    return;

  if (iopoll (-1, STDOUT_FILENO, false) == IOPOLL_BROKEN_OUTPUT)
    die_pipe ();
}

MAYBE_UNUSED static bool
valid_file_spec (struct File_spec const *f)
{
  /* Exactly one of the following subexpressions must be true. */
  return (f->fd < 0) ^ (f->errnum == 0);
}

/* Call lseek with the specified arguments FD, OFFSET, WHENCE.
   FD corresponds to PRETTYNAME.
   Give a diagnostic and exit nonzero if lseek fails.
   Otherwise, return the resulting offset.  */

static off_t
xlseek (int fd, off_t offset, int whence, char const *prettyname)
{
  off_t new_offset = lseek (fd, offset, whence);

  if (0 <= new_offset)
    return new_offset;

  static char const *whence_msgid[] = {
    [SEEK_SET] = N_("%s: cannot seek to offset %jd"),
    [SEEK_CUR] = N_("%s: cannot seek to relative offset %jd"),
    [SEEK_END] = N_("%s: cannot seek to end-relative offset %jd")
  };
  intmax_t joffset = offset;
  error (EXIT_FAILURE, errno, gettext (whence_msgid[whence]),
         quotef (prettyname), joffset);
}

/* Record a file F with descriptor FD, read position READ_POS, status ST,
   and blocking status BLOCKING.  READ_POS matters only for regular files,
   and READ_POS < 0 means the position is unknown.  */

static void
record_open_fd (struct File_spec *f, int fd,
                off_t read_pos, struct stat const *st,
                int blocking)
{
  f->fd = fd;
  f->mtime = get_stat_mtime (st);
  f->st_dev = st->st_dev;
  f->st_ino = st->st_ino;
  f->mode = st->st_mode;
  if (S_ISREG (st->st_mode))
    f->read_pos = (read_pos < 0
                   ? xlseek (fd, 0, SEEK_CUR, f->prettyname)
                   : read_pos);
  f->blocking = blocking;
  f->n_unchanged_stats = 0;
  f->ignore = false;
}

/* Close the file with descriptor FD and spec F.  */

static void
close_fd (int fd, struct File_spec const *f)
{
  if (STDIN_FILENO < fd && close (fd) < 0)
    error (0, errno, _("closing %s (fd=%d)"), quoteaf (f->prettyname), fd);
}

static void
write_header (char const *prettyname)
{
  static bool first_file = true;

  printf ("%s==> %s <==\n", first_file ? "" : "\n", prettyname);
  first_file = false;
}

/* Write N_BYTES from BUFFER to stdout.
   Exit immediately on error with a single diagnostic.  */

static void
xwrite_stdout (char const *buffer, idx_t n_bytes)
{
  if (n_bytes > 0 && fwrite (buffer, 1, n_bytes, stdout) < n_bytes)
    {
      clearerr (stdout); /* To avoid redundant close_stdout diagnostic.  */
      error (EXIT_FAILURE, errno, _("error writing %s"),
             quoteaf ("standard output"));
    }
}

/* Read and output bytes from a file, starting at its current position.
   If WANT_HEADER, if there is output then output a header before it.
   The file's name is PRETTYNAME and its file descriptor is FD.
   If N_BYTES is nonnegative, read and output at most N_BYTES.
   If N_BYTES is COPY_TO_EOF, copy until end of file.
   If N_BYTES is COPY_A_BUFFER, copy at most one buffer's worth.
   Return the number of bytes read from the file.  */

static count_t
dump_remainder (bool want_header, char const *prettyname, int fd,
                count_t n_bytes)
{
  count_t n_read = 0;
  count_t n_remaining = n_bytes;

  do
    {
      char buffer[BUFSIZ];
      idx_t n = n_bytes < 0 || BUFSIZ < n_remaining ? BUFSIZ : n_remaining;
      ssize_t bytes_read = read (fd, buffer, n);
      if (bytes_read < 0)
        {
          if (errno != EAGAIN)
            error (EXIT_FAILURE, errno, _("error reading %s"),
                   quoteaf (prettyname));
          break;
        }
      if (bytes_read == 0)
        break;
      n_read += bytes_read;

      if (want_header)
        {
          write_header (prettyname);
          want_header = false;
        }
      xwrite_stdout (buffer, bytes_read);
      if (n_bytes == COPY_A_BUFFER)
        break;

      n_remaining -= bytes_read;
      /* n_remaining < 0 if n_bytes == COPY_TO_EOF.  */
    }
  while (n_remaining != 0);

  return n_read;
}

/* Print the last N_LINES lines from the end of file FD.
   Go backward through the file, reading 'BUFSIZ' bytes at a time (except
   probably the first), until we hit the start of the file or have
   read NUMBER newlines.
   START_POS is the starting position of the read pointer for the file
   associated with FD (may be nonzero).
   END_POS is the file offset of EOF (one larger than offset of last byte).
   The file is a regular file positioned at END_POS.
   Return (-1 - errno) on failure, otherwise the resulting file offset,
   which can be less than READ_POS if the file shrank.  */

static off_t
file_lines (char const *prettyname, int fd, struct stat const *sb,
            count_t n_lines, off_t start_pos, off_t end_pos)
{
  char *buffer;
  idx_t bufsize = BUFSIZ;
  off_t pos = end_pos;

  if (n_lines == 0)
    return pos;

  /* Be careful with files with sizes that are a multiple of the page size,
     as on /proc or /sys file systems these files accept seeking to within
     the file, but then return no data when read.  So use a buffer that's
     at least PAGE_SIZE to avoid seeking within such files.

     We could also indirectly use a large enough buffer through io_blksize()
     however this would be less efficient in the common case, as it would
     generally pick a larger buffer size, resulting in reading more data
     from the end of the file.  */
  affirm (S_ISREG (sb->st_mode));
  if (sb->st_size % page_size == 0)
    bufsize = MAX (BUFSIZ, page_size);

  buffer = ximalloc (bufsize);

  /* Set 'bytes_read' to the size of the last, probably partial, buffer;
     0 < 'bytes_read' <= 'bufsize'.  */
  idx_t bytes_to_read = (pos - start_pos) % bufsize;
  if (bytes_to_read == 0)
    bytes_to_read = bufsize;
  /* Make 'pos' a multiple of 'bufsize' (0 if the file is short), so that all
     reads will be on block boundaries, which might increase efficiency.  */
  pos = xlseek (fd, -bytes_to_read, SEEK_CUR, prettyname);
  ssize_t bytes_read = read (fd, buffer, bytes_to_read);
  if (bytes_read < 0)
    {
      pos = -1 - errno;
      error (0, errno, _("error reading %s"), quoteaf (prettyname));
      goto free_buffer;
    }

  /* Count the incomplete line on files that don't end with a newline.  */
  if (bytes_read && buffer[bytes_read - 1] != line_end)
    --n_lines;

  do
    {
      pos += bytes_read;

      /* Scan backward, counting the newlines in this bufferfull.  */

      idx_t n = bytes_read;
      while (n)
        {
          char const *nl;
          nl = memrchr (buffer, line_end, n);
          if (nl == nullptr)
            break;
          n = nl - buffer;
          if (n_lines-- == 0)
            {
              /* If this newline isn't the last character in the buffer,
                 output the part that is after it.  */
              xwrite_stdout (nl + 1, bytes_read - (n + 1));
              pos += dump_remainder (false, prettyname, fd, end_pos - pos);
              goto free_buffer;
            }
        }

      /* Not enough newlines in that bufferfull.  */
      if (pos - bytes_read == start_pos)
        {
          /* Not enough lines in the file; print everything from
             start_pos to the end.  */
          xwrite_stdout (buffer, bytes_read);
          dump_remainder (false, prettyname, fd, end_pos - pos);
          goto free_buffer;
        }

      pos = xlseek (fd, -(bufsize + bytes_read), SEEK_CUR, prettyname);
      bytes_read = read (fd, buffer, bufsize);
      if (bytes_read < 0)
        {
          pos = -1 - errno;
          error (0, errno, _("error reading %s"), quoteaf (prettyname));
          goto free_buffer;
        }
    }
  while (bytes_read > 0);

free_buffer:
  free (buffer);
  return pos;
}

/* Print the last N_LINES lines from the end of the standard input,
   open for reading as pipe FD.
   Buffer the text as a linked list of LBUFFERs, adding them as needed.
   Return -1 if successful, (-1 - errno) otherwise.  */

static int
pipe_lines (char const *prettyname, int fd, count_t n_lines)
{
  struct linebuffer
  {
    char buffer[BUFSIZ];
    idx_t nbytes;
    idx_t nlines;
    struct linebuffer *next;
  };
  typedef struct linebuffer LBUFFER;
  LBUFFER *first, *last, *tmp;
  idx_t total_lines = 0;	/* Total number of newlines in all buffers.  */
  int ret = -1;
  ssize_t n_read;		/* Size in bytes of most recent read */

  first = last = xmalloc (sizeof (LBUFFER));
  first->nbytes = first->nlines = 0;
  first->next = nullptr;
  tmp = xmalloc (sizeof (LBUFFER));

  /* Input is always read into a fresh buffer.  */
  while (true)
    {
      n_read = read (fd, tmp->buffer, BUFSIZ);
      if (n_read <= 0)
        break;
      tmp->nbytes = n_read;
      tmp->nlines = 0;
      tmp->next = nullptr;

      /* Count the number of newlines just read.  */
      {
        char const *buffer_end = tmp->buffer + n_read;
        char const *p = tmp->buffer;
        while ((p = memchr (p, line_end, buffer_end - p)))
          {
            ++p;
            ++tmp->nlines;
          }
      }
      total_lines += tmp->nlines;

      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, 'n_read' can
         often be very small.  */
      if (tmp->nbytes + last->nbytes < BUFSIZ)
        {
          memcpy (&last->buffer[last->nbytes], tmp->buffer, tmp->nbytes);
          last->nbytes += tmp->nbytes;
          last->nlines += tmp->nlines;
        }
      else
        {
          /* If there's not enough room, link the new buffer onto the end of
             the list, then either free up the oldest buffer for the next
             read if that would leave enough lines, or else malloc a new one.
             Some compaction mechanism is possible but probably not
             worthwhile.  */
          last = last->next = tmp;
          if (total_lines - first->nlines > n_lines)
            {
              tmp = first;
              total_lines -= first->nlines;
              first = first->next;
            }
          else
            tmp = xmalloc (sizeof (LBUFFER));
        }
    }

  free (tmp);

  if (n_read < 0 && errno != EAGAIN)
    {
      ret = -1 - errno;
      error (0, errno, _("error reading %s"), quoteaf (prettyname));
      goto free_lbuffers;
    }

  /* If the file is empty, then bail out.  */
  if (last->nbytes == 0)
    goto free_lbuffers;

  /* This prevents a core dump when the pipe contains no newlines.  */
  if (n_lines == 0)
    goto free_lbuffers;

  /* Count the incomplete line on files that don't end with a newline.  */
  if (last->buffer[last->nbytes - 1] != line_end)
    {
      ++last->nlines;
      ++total_lines;
    }

  /* Run through the list, printing lines.  First, skip over unneeded
     buffers.  */
  for (tmp = first; total_lines - tmp->nlines > n_lines; tmp = tmp->next)
    total_lines -= tmp->nlines;

  /* Find the correct beginning, then print the rest of the file.  */
  {
    char const *beg = tmp->buffer;
    char const *buffer_end = tmp->buffer + tmp->nbytes;
    if (total_lines > n_lines)
      {
        /* Skip 'total_lines' - 'n_lines' newlines.  We made sure that
           'total_lines' - 'n_lines' <= 'tmp->nlines'.  */
        idx_t j;
        for (j = total_lines - n_lines; j; --j)
          {
            beg = rawmemchr (beg, line_end);
            ++beg;
          }
      }

    xwrite_stdout (beg, buffer_end - beg);
  }

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite_stdout (tmp->buffer, tmp->nbytes);

free_lbuffers:
  while (first)
    {
      tmp = first->next;
      free (first);
      first = tmp;
    }
  return ret;
}

/* Print the last N_BYTES characters from the end of FD.
   Work even if the input is a pipe.
   This is a stripped down version of pipe_lines.
   The initial file offset is READ_POS if nonnegative, otherwise unknown.
   Return (-1 - errno) on failure, otherwise the resulting file offset
   if known, otherwise -1.  */

static off_t
pipe_bytes (char const *prettyname, int fd, count_t n_bytes,
            off_t read_pos)
{
  struct charbuffer
  {
    char buffer[BUFSIZ];
    idx_t nbytes;
    struct charbuffer *next;
  };
  typedef struct charbuffer CBUFFER;
  CBUFFER *first, *last, *tmp;
  idx_t i;			/* Index into buffers.  */
  intmax_t total_bytes = 0;	/* Total characters in all buffers.  */
  ssize_t n_read;

  if (read_pos < 0)
    read_pos = TYPE_MINIMUM (off_t);

  first = last = xmalloc (sizeof (CBUFFER));
  first->nbytes = 0;
  first->next = nullptr;
  tmp = xmalloc (sizeof (CBUFFER));

  /* Input is always read into a fresh buffer.  */
  while (true)
    {
      n_read = read (fd, tmp->buffer, BUFSIZ);
      if (n_read <= 0)
        break;
      read_pos += n_read;
      tmp->nbytes = n_read;
      tmp->next = nullptr;

      total_bytes += tmp->nbytes;
      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, 'nbytes' can
         often be very small.  */
      if (tmp->nbytes + last->nbytes < BUFSIZ)
        {
          memcpy (&last->buffer[last->nbytes], tmp->buffer, tmp->nbytes);
          last->nbytes += tmp->nbytes;
        }
      else
        {
          /* If there's not enough room, link the new buffer onto the end of
             the list, then either free up the oldest buffer for the next
             read if that would leave enough characters, or else malloc a new
             one.  Some compaction mechanism is possible but probably not
             worthwhile.  */
          last = last->next = tmp;
          if (total_bytes - first->nbytes > n_bytes)
            {
              tmp = first;
              total_bytes -= first->nbytes;
              first = first->next;
            }
          else
            {
              tmp = xmalloc (sizeof (CBUFFER));
            }
        }
    }

  free (tmp);

  if (n_read < 0 && errno != EAGAIN)
    {
      read_pos = -1 - errno;
      error (0, errno, _("error reading %s"), quoteaf (prettyname));
      goto free_cbuffers;
    }

  /* Run through the list, printing characters.  First, skip over unneeded
     buffers.  */
  for (tmp = first; total_bytes - tmp->nbytes > n_bytes; tmp = tmp->next)
    total_bytes -= tmp->nbytes;

  /* Find the correct beginning, then print the rest of the file.
     We made sure that 'total_bytes' - 'n_bytes' <= 'tmp->nbytes'.  */
  if (total_bytes > n_bytes)
    i = total_bytes - n_bytes;
  else
    i = 0;
  xwrite_stdout (&tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite_stdout (tmp->buffer, tmp->nbytes);

  if (read_pos < 0)
    read_pos = -1;

free_cbuffers:
  while (first)
    {
      tmp = first->next;
      free (first);
      first = tmp;
    }
  return read_pos;
}

/* Skip N_BYTES characters from the start of pipe FD, and print
   any extra characters that were read beyond that.
   Return (-1 - errno) on failure, -1 if EOF, 0 otherwise.  */

static int
start_bytes (char const *prettyname, int fd, count_t n_bytes)
{
  char buffer[BUFSIZ];

  while (0 < n_bytes)
    {
      ssize_t bytes_read = read (fd, buffer, BUFSIZ);
      if (bytes_read == 0)
        return -1;
      if (bytes_read < 0)
        {
          int ret = -1 - errno;
          error (0, errno, _("error reading %s"), quoteaf (prettyname));
          return ret;
        }
      if (bytes_read <= n_bytes)
        n_bytes -= bytes_read;
      else
        {
          xwrite_stdout (&buffer[n_bytes], bytes_read - n_bytes);
          break;
        }
    }

  return 0;
}

/* Skip N_LINES lines at the start of file or pipe FD, and print
   any extra bytes that were read beyond that.
   Return (-1 - errno) on failure, -1 if EOF, 0 otherwise.  */

static int
start_lines (char const *prettyname, int fd, count_t n_lines)
{
  if (n_lines == 0)
    return 0;

  while (true)
    {
      char buffer[BUFSIZ];
      ssize_t bytes_read = read (fd, buffer, BUFSIZ);
      if (bytes_read == 0) /* EOF */
        return -1;
      if (bytes_read < 0) /* error */
        {
          int ret = -1 - errno;
          error (0, errno, _("error reading %s"), quoteaf (prettyname));
          return ret;
        }

      char *buffer_end = buffer + bytes_read;
      char *p = buffer;
      while ((p = memchr (p, line_end, buffer_end - p)))
        {
          ++p;
          if (--n_lines == 0)
            {
              if (p < buffer_end)
                xwrite_stdout (p, buffer_end - p);
              return 0;
            }
        }
    }
}

/* Return true if FD might be open on a file residing on a remote file system,
   false if FD is known to be local.  F is the file's spec.
   If fstatfs fails, give a diagnostic and return true.
   If fstatfs cannot be called, return true.  */
static bool
fremote (int fd, struct File_spec const *f)
{
  bool remote = true;           /* be conservative (poll by default).  */

#if HAVE_FSTATFS && HAVE_STRUCT_STATFS_F_TYPE \
 && (defined __linux__ || defined __ANDROID__)
  struct statfs buf;
  int err = fstatfs (fd, &buf);
  if (err != 0)
    {
      /* On at least linux-2.6.38, fstatfs fails with ENOSYS when FD
         is open on a pipe.  Treat that like a remote file.  */
      if (errno != ENOSYS)
        error (0, errno, _("cannot determine location of %s. "
                           "reverting to polling"), quoteaf (f->prettyname));
    }
  else
    {
      /* Treat unrecognized file systems as "remote", so caller polls.
         Note README-release has instructions for syncing the internal
         list with the latest Linux kernel file system constants.  */
      remote = is_local_fs_type (buf.f_type) <= 0;
    }
#endif

  return remote;
}

/* open/fstat F->name and handle changes.  */
static void
recheck (struct File_spec *f, bool blocking)
{
  struct stat new_stats;
  bool ok = false;
  bool is_stdin = (streq (f->name, "-"));
  int prev_errnum = f->errnum;
  bool new_file;
  int fd = (is_stdin
            ? STDIN_FILENO
            : open (f->name, O_RDONLY | (blocking ? 0 : O_NONBLOCK)));
  int open_errno = fd < 0 ? errno : 0;

  affirm (valid_file_spec (f));

  if (! disable_inotify && issymlink (f->name) == 1)
    {
      /* Diagnose the edge case where a regular file is changed
         to a symlink.  We avoid inotify with symlinks since
         it's awkward to match between symlink name and target.  */
      f->errnum = -1;
      f->ignore = true;

      error (0, 0, _("%s has been replaced with an untailable symbolic link"),
             quoteaf (f->prettyname));
    }
  else if (fd < 0 || fstat (fd, &new_stats) < 0)
    {
      f->errnum = fd < 0 ? open_errno : errno;
      if (fd < 0)
        {
          if (f->tailable)
            {
              /* FIXME-maybe: detect the case in which the file first becomes
                 unreadable (perms), and later becomes readable again and can
                 be seen to be the same file (dev/ino).  Otherwise, tail prints
                 the entire contents of the file when it becomes readable.  */
              error (0, f->errnum, _("%s has become inaccessible"),
                     quoteaf (f->prettyname));
            }
          else
            {
              /* say nothing... it's still not tailable */
            }
        }
      else if (prev_errnum != f->errnum)
        error (0, f->errnum, "%s", quotef (f->prettyname));
    }
  else if (!IS_TAILABLE_FILE_TYPE (new_stats.st_mode))
    {
      f->errnum = -1;
      f->ignore = ! (reopen_inaccessible_files && follow_mode == Follow_name);
      if (f->tailable || ! (prev_errnum < 0 || prev_errnum == EISDIR))
        error (0, 0, _("%s has been replaced with an untailable file%s"),
               quoteaf (f->prettyname),
               f->ignore ? _("; giving up on this name") : "");
    }
  else if ((f->remote = fremote (fd, f)) && ! disable_inotify)
    {
      f->errnum = -1;
      error (0, 0, _("%s has been replaced with an untailable remote file"),
             quoteaf (f->prettyname));
      f->ignore = true;
    }
  else
    {
      ok = true;
      f->errnum = 0;
    }

  f->tailable = ok;

  new_file = false;
  if (!ok)
    {
      close_fd (fd, f);
      close_fd (f->fd, f);
      f->fd = -1;
    }
  else if (prev_errnum && prev_errnum != ENOENT)
    {
      new_file = true;
      affirm (f->fd < 0);
      error (0, 0, _("%s has become accessible"), quoteaf (f->prettyname));
    }
  else if (f->fd < 0)
    {
      /* A new file even when inodes haven't changed as <dev,inode>
         pairs can be reused, and we know the file was missing
         on the previous iteration.  Note this also means the file
         is redisplayed in --follow=name mode if renamed away from
         and back to a monitored name.  */
      new_file = true;

      error (0, 0,
             _("%s has appeared;  following new file"),
             quoteaf (f->prettyname));
    }
  else if (!SAME_INODE (*f, new_stats))
    {
      /* File has been replaced (e.g., via log rotation) --
        tail the new one.  */
      new_file = true;

      error (0, 0,
             _("%s has been replaced;  following new file"),
             quoteaf (f->prettyname));

      /* Close the old one.  */
      close_fd (f->fd, f);
    }
  else
    {
      /* No changes detected, so close new fd.  */
      close_fd (fd, f);
    }

  /* FIXME: When a log is rotated, daemons tend to log to the
     old file descriptor until the new file is present and
     the daemon is sent a signal.  Therefore tail may miss entries
     being written to the old file.  Perhaps we should keep
     the older file open and continue to monitor it until
     data is written to a new file.  */
  if (new_file)
    {
      /* Start at the file's current position, normally the beginning.  */
      record_open_fd (f, fd, -1, &new_stats, is_stdin ? -1 : blocking);
    }
}

/* Return true if any of the N_FILES files in F are live, i.e., have
   open file descriptors, or should be checked again (see --retry).
   When following descriptors, checking should only continue when any
   of the files is not yet ignored.  */

static bool
any_live_files (const struct File_spec *f, int n_files)
{
  /* In inotify mode, ignore may be set for files
     which may later be replaced with new files.
     So always consider files live in -F mode.  */
  if (reopen_inaccessible_files && follow_mode == Follow_name)
    return true;

  for (int i = 0; i < n_files; i++)
    {
      if (0 <= f[i].fd)
        return true;
      else
        {
          if (! f[i].ignore && reopen_inaccessible_files)
            return true;
        }
    }

  return false;
}

/* Return true if any writers exist, even as zombies.  */

static bool
some_writers_exist (void)
{
  /* To avoid some (though not all) races if the kernel reuses PIDs,
     check all PIDs instead of returning merely because one PID exists.  */
  for (idx_t i = 0; i < nbpids; )
    {
      if (kill (pids[i], 0) < 0 && errno == ESRCH)
        {
          nbpids--;
          memmove (&pids[i], &pids[i + 1], (nbpids - i) * sizeof pids[i]);
        }
      else
        i++;
    }


  return 0 < nbpids;
}

/* Tail N_FILES files forever, or until killed.
   The pertinent information for each file is stored in an entry of F.
   Loop over each of them, doing an fstat to see if they have changed size,
   and an occasional open/fstat to see if any dev/ino pair has changed.
   If none of them have changed size in one iteration, sleep for a
   while and try again.  Continue until the user interrupts us.  */

static void
tail_forever (struct File_spec *f, int n_files, double sleep_interval)
{
  int last = n_files - 1;

  while (true)
    {
      /* Use blocking I/O as an optimization, when it's easy.  */
      bool blocking = (!nbpids && follow_mode == Follow_descriptor
                       && n_files == 1 && 0 <= f[0].fd && !S_ISREG (f[0].mode));

      bool any_input = false;

      for (int i = 0; i < n_files; i++)
        {
          struct stat stats;

          if (f[i].ignore)
            continue;

          int fd = f[i].fd;
          if (fd < 0)
            {
              recheck (&f[i], blocking);
              continue;
            }

          char const *prettyname = f[i].prettyname;
          mode_t mode = f[i].mode;

          if (f[i].blocking != blocking)
            {
              int old_flags = fcntl (fd, F_GETFL);
              int new_flags = old_flags | (blocking ? 0 : O_NONBLOCK);
              if (old_flags < 0
                  || (new_flags != old_flags
                      && fcntl (fd, F_SETFL, new_flags) < 0))
                {
                  /* Don't update f[i].blocking if fcntl fails.  */
                  if (S_ISREG (f[i].mode) && errno == EPERM)
                    {
                      /* This happens when using tail -f on a file with
                         the append-only attribute.  */
                    }
                  else
                    error (EXIT_FAILURE, errno,
                           _("%s: cannot change nonblocking mode"),
                           quotef (prettyname));
                }
              else
                f[i].blocking = blocking;
            }

          bool read_unchanged = false;
          if (!f[i].blocking)
            {
              if (fstat (fd, &stats) < 0)
                {
                  f[i].fd = -1;
                  f[i].errnum = errno;
                  error (0, errno, "%s", quotef (prettyname));
                  close (fd); /* ignore failure */
                  continue;
                }

              if (f[i].mode == stats.st_mode
                  && (! S_ISREG (stats.st_mode)
                      || f[i].read_pos == stats.st_size)
                  && timespec_cmp (f[i].mtime, get_stat_mtime (&stats)) == 0)
                {
                  if ((max_n_unchanged_stats_between_opens
                       <= f[i].n_unchanged_stats++)
                      && follow_mode == Follow_name)
                    {
                      recheck (&f[i], f[i].blocking);
                      f[i].n_unchanged_stats = 0;
                    }
                  if (fd != f[i].fd || S_ISREG (stats.st_mode) || 1 < n_files)
                    continue;
                  else
                    read_unchanged = true;
                }

              affirm (fd == f[i].fd);

              /* This file has changed.  Print out what we can, and
                 then keep looping.  */

              f[i].mtime = get_stat_mtime (&stats);
              f[i].mode = stats.st_mode;

              /* reset counter */
              if (! read_unchanged)
                f[i].n_unchanged_stats = 0;

              /* XXX: This is only a heuristic, as the file may have also
                 been truncated and written to if st_size >= size
                 (in which case we ignore new data <= size).  */
              if (S_ISREG (mode) && stats.st_size < f[i].read_pos)
                {
                  error (0, 0, _("%s: file truncated"),
                         quotef (prettyname));
                  /* Assume the file was truncated to 0,
                     and therefore output all "new" data.  */
                  f[i].read_pos = xlseek (fd, 0, SEEK_SET, prettyname);
                }

              if (i != last)
                {
                  if (print_headers)
                    write_header (prettyname);
                  last = i;
                }
            }

          /* Don't read more than st_size on networked file systems
             because it was seen on glusterfs at least, that st_size
             may be smaller than the data read on a _subsequent_ stat call.  */
          count_t bytes_to_read;
          if (f[i].blocking)
            bytes_to_read = COPY_A_BUFFER;
          else if (S_ISREG (mode) && f[i].remote)
            bytes_to_read = stats.st_size - f[i].read_pos;
          else
            bytes_to_read = COPY_TO_EOF;

          count_t nr = dump_remainder (false, prettyname, fd, bytes_to_read);
          if (0 < nr)
            {
              if (S_ISREG (mode))
                f[i].read_pos += nr;
              if (read_unchanged)
                f[i].n_unchanged_stats = 0;
              any_input = true;
            }
        }

      if (! any_live_files (f, n_files))
        {
          error (EXIT_FAILURE, 0, _("no files remaining"));
          break;
        }

      if ((!any_input || blocking) && fflush (stdout) < 0)
        write_error ();

      check_output_alive ();

      if (!any_input)
        {
          if (pids)
            {
              /* Exit loop if it was already known that no writers exist.
                 Otherwise, to avoid a race, loop one more time
                 without sleeping if no writers exist now.  */
              if (!nbpids)
                break;
              if (!some_writers_exist ())
                continue;
            }

          if (xnanosleep (sleep_interval))
            error (EXIT_FAILURE, errno, _("cannot read realtime clock"));
        }
    }
}

#if HAVE_INOTIFY

/* Return true if any of the N_FILES files in F is remote, i.e., has
   an open file descriptor and is on a network file system.  */

static bool
any_remote_file (const struct File_spec *f, int n_files)
{
  for (int i = 0; i < n_files; i++)
    if (0 <= f[i].fd && f[i].remote)
      return true;
  return false;
}

/* Return true if any of the N_FILES files in F is non remote, i.e., has
   an open file descriptor and is not on a network file system.  */

static bool
any_non_remote_file (const struct File_spec *f, int n_files)
{
  for (int i = 0; i < n_files; i++)
    if (0 <= f[i].fd && ! f[i].remote)
      return true;
  return false;
}

/* Return true if any of the N_FILES files in F is a symlink.
   Note we don't worry about the edge case where "-" exists,
   since that will have the same consequences for inotify,
   which is the only context this function is currently used.  */

static bool
any_symlinks (const struct File_spec *f, int n_files)
{
  for (int i = 0; i < n_files; i++)
    if (issymlink (f[i].name) == 1)
      return true;
  return false;
}

/* Return true if any of the N_FILES files in F is not
   a regular file or fifo.  This is used to avoid adding inotify
   watches on a device file for example, which inotify
   will accept, but not give any events for.  */

static bool
any_non_regular_fifo (const struct File_spec *f, int n_files)
{
  for (int i = 0; i < n_files; i++)
    if (0 <= f[i].fd && ! S_ISREG (f[i].mode) && ! S_ISFIFO (f[i].mode))
      return true;
  return false;
}

/* Return true if any of the N_FILES files in F represents
   stdin and is not ignored.  */

static bool
tailable_stdin (const struct File_spec *f, int n_files)
{
  for (int i = 0; i < n_files; i++)
    if (!f[i].ignore && streq (f[i].name, "-"))
      return true;
  return false;
}

static size_t
wd_hasher (const void *entry, size_t tabsize)
{
  const struct File_spec *spec = entry;
  return spec->wd % tabsize;
}

static bool
wd_comparator (const void *e1, const void *e2)
{
  const struct File_spec *spec1 = e1;
  const struct File_spec *spec2 = e2;
  return spec1->wd == spec2->wd;
}

/* Output (new) data for FSPEC->fd.
   PREV_FSPEC records the last File_spec for which we output.  */
static void
check_fspec (struct File_spec *fspec, struct File_spec **prev_fspec)
{
  if (fspec->fd < 0)
    return;

  struct stat stats;
  if (fstat (fspec->fd, &stats) < 0)
    {
      fspec->errnum = errno;
      close_fd (fspec->fd, fspec);
      fspec->fd = -1;
      return;
    }

  char const *prettyname = fspec->prettyname;

  /* XXX: This is only a heuristic, as the file may have also
     been truncated and written to if st_size >= size
     (in which case we ignore new data <= size).
     Though in the inotify case it's more likely we'll get
     separate events for truncate() and write().  */
  if (S_ISREG (fspec->mode) && stats.st_size < fspec->read_pos)
    {
      error (0, 0, _("%s: file truncated"), quotef (prettyname));
      fspec->read_pos = xlseek (fspec->fd, 0, SEEK_SET, prettyname);
    }
  else if (S_ISREG (fspec->mode) && stats.st_size == fspec->read_pos
           && timespec_cmp (fspec->mtime, get_stat_mtime (&stats)) == 0)
    return;

  bool want_header = print_headers && (fspec != *prev_fspec);

  count_t nr = dump_remainder (want_header, prettyname, fspec->fd, COPY_TO_EOF);
  if (0 < nr)
    {
      if (S_ISREG (fspec->mode))
        fspec->read_pos += nr;
      *prev_fspec = fspec;
      if (fflush (stdout) < 0)
        write_error ();
    }
}

/* Attempt to tail N_FILES files forever, or until killed.
   Check modifications using the inotify events system.
   Exit if finished or on fatal error; return to revert to polling.  */
static void
tail_forever_inotify (int wd, struct File_spec *f, int n_files,
                      double sleep_interval, Hash_table **wd_to_namep)
{
# if TAIL_TEST_SLEEP
  /* Delay between open() and inotify_add_watch()
     to help trigger different cases.  */
  xnanosleep (1000000);
# endif
  int max_realloc = 3;

  /* Map an inotify watch descriptor to the name of the file it's watching.  */
  Hash_table *wd_to_name;

  bool found_watchable_file = false;
  bool tailed_but_unwatchable = false;
  bool found_unwatchable_dir = false;
  bool no_inotify_resources = false;
  struct File_spec *prev_fspec;
  idx_t evlen = 0;
  char *evbuf;
  idx_t evbuf_off = 0;

  wd_to_name = hash_initialize (n_files, nullptr, wd_hasher, wd_comparator,
                                nullptr);
  if (! wd_to_name)
    xalloc_die ();
  *wd_to_namep = wd_to_name;

  /* The events mask used with inotify on files (not directories).  */
  uint32_t inotify_wd_mask = IN_MODIFY;
  /* TODO: Perhaps monitor these events in Follow_descriptor mode also,
     to tag reported file names with "deleted", "moved" etc.  */
  if (follow_mode == Follow_name)
    inotify_wd_mask |= (IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF);

  /* Add an inotify watch for each watched file.  If -F is specified then watch
     its parent directory too, in this way when they re-appear we can add them
     again to the watch list.  */
  for (int i = 0; i < n_files; i++)
    {
      if (!f[i].ignore)
        {
          idx_t fnlen = strlen (f[i].name);
          if (evlen < fnlen)
            evlen = fnlen;

          f[i].wd = -1;

          if (follow_mode == Follow_name)
            {
              idx_t dirlen = dir_len (f[i].name);
              char prev = f[i].name[dirlen];
              f[i].basename_start = last_component (f[i].name) - f[i].name;

              f[i].name[dirlen] = '\0';

               /* It's fine to add the same directory more than once.
                  In that case the same watch descriptor is returned.  */
              f[i].parent_wd = inotify_add_watch (wd, dirlen ? f[i].name : ".",
                                                  (IN_CREATE | IN_DELETE
                                                   | IN_MOVED_TO | IN_ATTRIB
                                                   | IN_DELETE_SELF));

              f[i].name[dirlen] = prev;

              if (f[i].parent_wd < 0)
                {
                  if (errno != ENOSPC) /* suppress confusing error.  */
                    error (0, errno, _("cannot watch parent directory of %s"),
                           quoteaf (f[i].name));
                  else
                    error (0, 0, _("inotify resources exhausted"));
                  found_unwatchable_dir = true;
                  /* We revert to polling below.  Note invalid uses
                     of the inotify API will still be diagnosed.  */
                  break;
                }
            }

          f[i].wd = inotify_add_watch (wd, f[i].name, inotify_wd_mask);

          if (f[i].wd < 0)
            {
              if (0 <= f[i].fd)  /* already tailed.  */
                tailed_but_unwatchable = true;
              if (errno == ENOSPC || errno == ENOMEM)
                {
                  no_inotify_resources = true;
                  error (0, 0, _("inotify resources exhausted"));
                  break;
                }
              else if (errno != f[i].errnum)
                error (0, errno, _("cannot watch %s"), quoteaf (f[i].name));
              continue;
            }

          if (hash_insert (wd_to_name, &(f[i])) == nullptr)
            xalloc_die ();

          found_watchable_file = true;
        }
    }

  /* Linux kernel 2.6.24 at least has a bug where eventually, ENOSPC is always
     returned by inotify_add_watch.  In any case we should revert to polling
     when there are no inotify resources.  Also a specified directory may not
     be currently present or accessible, so revert to polling.  Also an already
     tailed but unwatchable due rename/unlink race, should also revert.  */
  if (no_inotify_resources || found_unwatchable_dir
      || (follow_mode == Follow_descriptor && tailed_but_unwatchable))
    return;
  if (follow_mode == Follow_descriptor && !found_watchable_file)
    exit (EXIT_FAILURE);

  prev_fspec = &(f[n_files - 1]);

  /* Check files again.  New files or data can be available since last time we
     checked and before they are watched by inotify.  */
  for (int i = 0; i < n_files; i++)
    {
      if (! f[i].ignore)
        {
          /* check for new files.  */
          if (follow_mode == Follow_name)
            recheck (&(f[i]), false);
          else if (0 <= f[i].fd)
            {
              /* If the file was replaced in the small window since we tailed,
                 then assume the watch is on the wrong item (different to
                 that we've already produced output for), and so revert to
                 polling the original descriptor.  */
              struct stat stats;

              if (! (stat (f[i].name, &stats) < 0
                     || SAME_INODE (f[i], stats)))
                {
                  error (0, errno, _("%s was replaced"),
                         quoteaf (f[i].prettyname));
                  return;
                }
            }

          /* check for new data.  */
          check_fspec (&f[i], &prev_fspec);
        }
    }

  evlen += sizeof (struct inotify_event) + 1;
  evbuf = ximalloc (evlen);

  /* Wait for inotify events and handle them.  Events on directories
     ensure that watched files can be re-added when following by name.
     This loop blocks on the 'read' call until a new event is notified.
     But when --pid=P is specified, tail usually waits via poll.  */
  ptrdiff_t len = 0;
  while (true)
    {
      struct File_spec *fspec;
      struct inotify_event *ev;
      void *void_ev;

      /* When following by name without --retry, and the last file has
         been unlinked or renamed-away, diagnose it and return.  */
      if (follow_mode == Follow_name
          && ! reopen_inaccessible_files
          && hash_get_n_entries (wd_to_name) == 0)
        error (EXIT_FAILURE, 0, _("no files remaining"));

      if (len <= evbuf_off)
        {
          /* Poll for inotify events.  When watching a PID, ensure
             that a read from WD will not block indefinitely.
             If MONITOR_OUTPUT, also poll for a broken output pipe.  */

          int file_change;
          struct pollfd pfd[2];
          do
            {
              /* How many ms to wait for changes.  -1 means wait forever.  */
              int delay = -1;

              if (pids)
                {
                  if (!nbpids)
                    exit (EXIT_SUCCESS);

                  if (!some_writers_exist () || sleep_interval <= 0)
                    delay = 0;
                  else if (sleep_interval < INT_MAX / 1000 - 1)
                    {
                      /* delay = ceil (sleep_interval * 1000), sans libm.  */
                      double ddelay = sleep_interval * 1000;
                      delay = ddelay;
                      delay += delay < ddelay;
                    }
                }

              pfd[0].fd = wd;
              pfd[0].events = POLLIN;
              pfd[1].fd = STDOUT_FILENO;
              pfd[1].events = pfd[1].revents = 0;
              file_change = poll (pfd, monitor_output + 1, delay);
            }
          while (file_change == 0);

          if (file_change < 0)
            error (EXIT_FAILURE, errno,
                   _("error waiting for inotify and output events"));
          if (pfd[1].revents)
            die_pipe ();

          len = read (wd, evbuf, evlen);
          evbuf_off = 0;

          /* For kernels prior to 2.6.21, read returns 0 when the buffer
             is too small.  */
          if ((len == 0 || (len < 0 && errno == EINVAL))
              && max_realloc--)
            {
              len = 0;
              evlen *= 2;
              evbuf = xirealloc (evbuf, evlen);
              continue;
            }

          if (len <= 0)
            error (EXIT_FAILURE, errno, _("error reading inotify event"));
        }

      void_ev = evbuf + evbuf_off;
      ev = void_ev;
      evbuf_off += sizeof (*ev) + ev->len;

      /* If a directory is deleted, IN_DELETE_SELF is emitted
         with ev->name of length 0.
         We need to catch it, otherwise it would wait forever,
         as wd for directory becomes inactive. Revert to polling now.   */
      if ((ev->mask & IN_DELETE_SELF) && ! ev->len)
        {
          for (int i = 0; i < n_files; i++)
            {
              if (ev->wd == f[i].parent_wd)
                {
                  error (0, 0,
                      _("directory containing watched file was removed"));
                  return;
                }
            }
        }

      if (ev->len) /* event on ev->name in watched directory.  */
        {
          int j;
          for (j = 0; j < n_files; j++)
            {
              /* With N=hundreds of frequently-changing files, this O(N^2)
                 process might be a problem.  FIXME: use a hash table?  */
              if (f[j].parent_wd == ev->wd
                  && streq (ev->name, f[j].name + f[j].basename_start))
                break;
            }

          /* It is not a watched file.  */
          if (j == n_files)
            continue;

          fspec = &(f[j]);

          int new_wd = -1;
          bool deleting = !! (ev->mask & IN_DELETE);

          if (! deleting)
            {
              /* Adding the same inode again will look up any existing wd.  */
              new_wd = inotify_add_watch (wd, f[j].name, inotify_wd_mask);
            }

          if (! deleting && new_wd < 0)
            {
              if (errno == ENOSPC || errno == ENOMEM)
                {
                  error (0, 0, _("inotify resources exhausted"));
                  return; /* revert to polling.  */
                }
              else
                {
                  /* Can get ENOENT for a dangling symlink for example.  */
                  error (0, errno, _("cannot watch %s"), quoteaf (f[j].name));
                }
              /* We'll continue below after removing the existing watch.  */
            }

          /* This will be false if only attributes of file change.  */
          bool new_watch;
          new_watch = (! deleting) && (fspec->wd < 0 || new_wd != fspec->wd);

          if (new_watch)
            {
              if (0 <= fspec->wd)
                {
                  inotify_rm_watch (wd, fspec->wd);
                  hash_remove (wd_to_name, fspec);
                }

              fspec->wd = new_wd;

              if (new_wd < 0)
                continue;

              /* If the file was moved then inotify will use the source file wd
                for the destination file.  Make sure the key is not present in
                the table.  */
              struct File_spec *prev = hash_remove (wd_to_name, fspec);
              if (prev && prev != fspec)
                {
                  if (follow_mode == Follow_name)
                    recheck (prev, false);
                  prev->wd = -1;
                  close_fd (prev->fd, prev);
                }

              if (hash_insert (wd_to_name, fspec) == nullptr)
                xalloc_die ();
            }

          if (follow_mode == Follow_name)
            recheck (fspec, false);
        }
      else
        {
          struct File_spec key;
          key.wd = ev->wd;
          fspec = hash_lookup (wd_to_name, &key);
        }

      if (! fspec)
        continue;

      if (ev->mask & (IN_ATTRIB | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF))
        {
          /* Note for IN_MOVE_SELF (the file we're watching has
             been clobbered via a rename) without --retry we leave the watch
             in place since it may still be part of the set
             of watched names.  */
          if (ev->mask & IN_DELETE_SELF
              || (!reopen_inaccessible_files && (ev->mask & IN_MOVE_SELF)))
            {
              inotify_rm_watch (wd, fspec->wd);
              hash_remove (wd_to_name, fspec);
            }

          /* Note we get IN_ATTRIB for unlink() as st_nlink decrements.
             The usual path is a close() done in recheck() triggers
             an IN_DELETE_SELF event as the inode is removed.
             However sometimes open() will succeed as even though
             st_nlink is decremented, the dentry (cache) is not updated.
             Thus we depend on the IN_DELETE event on the directory
             to trigger processing for the removed file.  */

          recheck (fspec, false);

          continue;
        }
      check_fspec (fspec, &prev_fspec);
    }
}
#endif

/* Output the last bytes of the file PRETTYNAME open for reading
   in FD and with status ST.  Output the last N_BYTES bytes.
   Return (-1 - errno) on failure, otherwise the resulting file offset
   if known, otherwise -1.  */

static off_t
tail_bytes (char const *prettyname, int fd, struct stat const *st,
            count_t n_bytes)
{
  off_t current_pos
    = (presume_input_pipe
       ? -1
       : lseek (fd, from_start ? MIN (n_bytes, OFF_T_MAX) : 0, SEEK_CUR));

  if (from_start)
    {
      if (current_pos < 0)
        {
          int t = start_bytes (prettyname, fd, n_bytes);
          if (t < 0)
            return t;
        }
      n_bytes = COPY_TO_EOF;
    }
  else
    {
      off_t initial_pos = current_pos;
      off_t end_pos = -1;

      if (0 <= current_pos)
        {
          if (usable_st_size (st))
            {
              /* Use st_size only if it's so large that this is
                 probably not a /proc or similar file, where st_size
                 is notional.  */
              off_t smallish_size = STP_BLKSIZE (st);
              if (smallish_size < st->st_size)
                end_pos = st->st_size;
            }
          else
            {
              off_t minus_n;
              if (ckd_sub (&minus_n, 0, n_bytes))
                end_pos = 0;
              else
                {
                  off_t e_n_pos = lseek (fd, minus_n, SEEK_END);
                  if (0 <= e_n_pos)
                    {
                      current_pos = e_n_pos;
                      end_pos = e_n_pos + n_bytes;
                    }
                  else
                    {
                      /* POSIX says errno is unreliable here.
                         Perhaps the file is smaller than N_BYTES.
                         Try again at file end.  */
                      off_t e_pos = lseek (fd, 0, SEEK_END);
                      if (0 <= e_pos)
                        current_pos = end_pos = e_pos;
                    }
               }
            }
        }

      /* If the end is known and more than N_BYTES after the initial
         position, go to N_BYTES before the end; otherwise go back to
         the initial position.  But do not lseek if lseek already failed.  */
      off_t pos = (initial_pos < end_pos && n_bytes < end_pos - initial_pos
                   ? end_pos - n_bytes
                   : initial_pos);
      if (pos != current_pos)
        current_pos = xlseek (fd, pos, SEEK_SET, prettyname);

      if (end_pos < 0)
        return pipe_bytes (prettyname, fd, n_bytes, current_pos);
    }

  count_t nr = dump_remainder (false, prettyname, fd, n_bytes);
  return current_pos < 0 ? -1 : current_pos + nr;
}

/* Output the last lines of the file PRETTYNAME open for reading
   in FD and with status ST.  Output the last N_LINES lines.
   Return (-1 - errno) on failure, otherwise the resulting file offset
   if known, otherwise -1.  */

static off_t
tail_lines (char const *prettyname, int fd, struct stat const *st,
            count_t n_lines)
{
  if (from_start)
    {
      /* If skipping all input use lseek if possible, for speed.  */
      if (OFF_T_MAX <= n_lines)
        {
          off_t e = lseek (fd, 0, SEEK_END);
          if (0 <= e)
            return e;
        }

      int t = start_lines (prettyname, fd, n_lines);
      if (t < 0)
        return t;
      dump_remainder (false, prettyname, fd, COPY_TO_EOF);
      return -1;
    }
  else
    {
      /* Use file_lines only if FD is a regular file and SEEK_END
         reports more data.  */
      off_t start_pos = (!presume_input_pipe && S_ISREG (st->st_mode)
                         ? lseek (fd, 0, SEEK_CUR)
                         : -1);
      off_t end_pos = start_pos < 0 ? -1 : lseek (fd, 0, SEEK_END);
      return (end_pos < 0
              ? pipe_lines (prettyname, fd, n_lines)
              : start_pos < end_pos
              ? file_lines (prettyname, fd, st, n_lines, start_pos, end_pos)
              : start_pos != end_pos
              ? (/* Do not read from before the start offset,
                    even if the input file shrank.  */
                 xlseek (fd, start_pos, SEEK_SET, prettyname))
              : start_pos);
    }
}

/* Display the last part of file FILENAME,
   open for reading via FD and with status *ST.
   Return (-1 - errno) on failure, otherwise the resulting file offset
   if known, otherwise -1.  */

static off_t
tail (char const *filename, int fd, struct stat const *st, count_t n_units)
{
  return (count_lines ? tail_lines : tail_bytes) (filename, fd, st, n_units);
}

/* Display the last N_UNITS units of the file described by F.
   Return true if successful.  */

static bool
tail_file (struct File_spec *f, count_t n_files, count_t n_units)
{
  int fd;
  bool ok;

  /* Avoid blocking if we may need to process asynchronously.  */
  bool nonblocking = forever && (nbpids || n_files > 1);

  bool is_stdin = (streq (f->name, "-"));

  if (is_stdin)
    {
      have_read_stdin = true;
      fd = STDIN_FILENO;
      xset_binary_mode (STDIN_FILENO, O_BINARY);
    }
  else
    fd = open (f->name, O_RDONLY | O_BINARY | (nonblocking ? O_NONBLOCK : 0));

  f->tailable = false;

  if (fd < 0)
    {
      if (forever)
        {
          f->fd = -1;
          f->errnum = errno;
          f->ignore = ! reopen_inaccessible_files;
          f->st_dev = 0;
          f->st_ino = 0;
        }
      error (0, errno, _("cannot open %s for reading"),
             quoteaf (f->prettyname));
      ok = false;
    }
  else
    {
      if (print_headers)
        write_header (f->prettyname);

      off_t read_pos;
      struct stat stats;
      ok = 0 <= fstat (fd, &stats);
      if (!ok)
        {
          f->errnum = errno;
          error (0, f->errnum, _("cannot fstat %s"), quoteaf (f->prettyname));
        }
      else
        {
          read_pos = tail (f->prettyname, fd, &stats, n_units);
          ok = -1 <= read_pos;
          f->errnum = ok ? 0 : -1 - read_pos;

          if (IS_TAILABLE_FILE_TYPE (stats.st_mode))
            f->tailable = true;
          else if (forever)
            {
              ok = false;
              f->errnum = -1;
              error (0, 0, _("%s: cannot follow end of this type of file%s"),
                     quotef (f->prettyname),
                     (reopen_inaccessible_files ? ""
                      : _("; giving up on this name")));
            }
        }

      if (forever)
        {
          if (!ok)
            {
              f->ignore = ! reopen_inaccessible_files;
              close_fd (fd, f);
              f->fd = -1;
            }
          else
            {
              record_open_fd (f, fd, read_pos, &stats, (is_stdin ? -1 : 1));
              f->remote = fremote (fd, f);
            }
        }
      else
        {
          if (!is_stdin && close (fd) < 0)
            {
              error (0, errno, _("error reading %s"),
                     quoteaf (f->prettyname));
              ok = false;
            }
        }
    }

  return ok;
}

/* If obsolete usage is allowed, and the command line arguments are of
   the obsolete form and the option string is well-formed, set
   *N_UNITS, the globals COUNT_LINES, FOREVER, and FROM_START, and
   return true.  If the command line arguments are obviously incorrect
   (e.g., because obsolete usage is not allowed and the arguments are
   incorrect for non-obsolete usage), report an error and exit.
   Otherwise, return false and don't modify any parameter or global
   variable.  */

static bool
parse_obsolete_option (int argc, char * const *argv, count_t *n_units)
{
  bool t_from_start;
  bool t_count_lines = true;
  bool t_forever = false;

  /* With the obsolete form, there is one option string and at most
     one file argument.  Watch out for "-" and "--", though.  */
  if (! (argc == 2
         || (argc == 3 && ! (argv[2][0] == '-' && argv[2][1]))
         || (3 <= argc && argc <= 4 && streq (argv[2], "--"))))
    return false;

  int posix_ver = posix2_version ();
  bool obsolete_usage = posix_ver < 200112;
  bool traditional_usage = obsolete_usage || 200809 <= posix_ver;
  char const *p = argv[1];

  switch (*p++)
    {
    default:
      return false;

    case '+':
      /* Leading "+" is a file name in the standard form.  */
      if (!traditional_usage)
        return false;

      t_from_start = true;
      break;

    case '-':
      /* In the non-obsolete form, "-" is standard input and "-c"
         requires an option-argument.  The obsolete multidigit options
         are supported as a GNU extension even when conforming to
         POSIX 1003.1-2001 or later, so don't complain about them.  */
      if (!obsolete_usage && !p[p[0] == 'c'])
        return false;

      t_from_start = false;
      break;
    }

  count_t n;
  if (!c_isdigit (*p))
    n = DEFAULT_N_LINES;
  else
    for (n = 0; c_isdigit (*p); p++)
      n = ckd_mul (&n, n, 10) || ckd_add (&n, n, *p - '0') ? COUNT_MAX : n;

  switch (*p)
    {
    case 'b': n = ckd_mul (&n, n, 512) ? COUNT_MAX : n; FALLTHROUGH;
    case 'c': t_count_lines = false; FALLTHROUGH;
    case 'l': p++; break;
    }

  if (*p == 'f')
    {
      t_forever = true;
      ++p;
    }

  if (*p)
    return false;

  *n_units = n;

  /* Set globals.  */
  from_start = t_from_start;
  count_lines = t_count_lines;
  forever = t_forever;

  return true;
}

static void
parse_options (int argc, char **argv,
               count_t *n_units, enum header_mode *header_mode,
               double *sleep_interval)
{
  int c;

  while ((c = getopt_long (argc, argv, "c:n:fFqs:vz0123456789",
                           long_options, nullptr))
         != -1)
    {
      switch (c)
        {
        case 'F':
          forever = true;
          follow_mode = Follow_name;
          reopen_inaccessible_files = true;
          break;

        case 'c':
        case 'n':
          count_lines = (c == 'n');
          if (*optarg == '+')
            from_start = true;
          else if (*optarg == '-')
            ++optarg;

          *n_units = xnumtoimax (optarg, 10, 0, COUNT_MAX, "bkKmMGTPEZYRQ0",
                                 (count_lines
                                  ? _("invalid number of lines")
                                  : _("invalid number of bytes"))
                                 , 0, XTOINT_MAX_QUIET);
          break;

        case 'f':
        case LONG_FOLLOW_OPTION:
          forever = true;
          if (optarg == nullptr)
            follow_mode = DEFAULT_FOLLOW_MODE;
          else
            follow_mode = XARGMATCH ("--follow", optarg,
                                     follow_mode_string, follow_mode_map);
          break;

        case RETRY_OPTION:
          reopen_inaccessible_files = true;
          break;

        case MAX_UNCHANGED_STATS_OPTION:
          /* --max-unchanged-stats=N */
          max_n_unchanged_stats_between_opens =
            xnumtoimax (optarg, 10, 0, COUNT_MAX, "",
                        _("invalid maximum number of unchanged stats"
                          " between opens"),
                        0, XTOINT_MAX_QUIET);
          break;

        case DISABLE_INOTIFY_OPTION:
          disable_inotify = true;
          break;

        case PID_OPTION:
          if (nbpids == pids_alloc)
            pids = xpalloc (pids, &pids_alloc, 1,
                            MIN (INT_MAX, IDX_MAX), sizeof *pids);
          pids[nbpids++] = xdectoumax (optarg, 0, PID_T_MAX, "",
                                       _("invalid PID"), 0);
          break;

        case PRESUME_INPUT_PIPE_OPTION:
          presume_input_pipe = true;
          break;

        case 'q':
          *header_mode = never;
          break;

        case 's':
          {
            char *ep;
            errno = 0;
            double s = cl_strtod (optarg, &ep);
            if (optarg == ep || *ep || ! (0 <= s))
              error (EXIT_FAILURE, 0,
                     _("invalid number of seconds: %s"), quote (optarg));
            *sleep_interval = dtimespec_bound (s, errno);
          }
          break;

        case 'v':
          *header_mode = always;
          break;

        case 'z':
          line_end = '\0';
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          error (EXIT_FAILURE, 0, _("option used in invalid context -- %c"), c);

        default:
          usage (EXIT_FAILURE);
        }
    }

  if (reopen_inaccessible_files)
    {
      if (!forever)
        {
          reopen_inaccessible_files = false;
          error (0, 0, _("warning: --retry ignored; --retry is useful"
                         " only when following"));
        }
      else if (follow_mode == Follow_descriptor)
        error (0, 0, _("warning: --retry only effective for the initial open"));
    }

  if (pids && (!forever || (kill (pids[0], 0) < 0 && errno == ENOSYS)))
    {
      error (0, 0,
             _(forever
               ? "warning: --pid=PID is not supported on this system"
               : ("warning: PID ignored;"
                  " --pid=PID is useful only when following")));
      free (pids);
      pids = nullptr;
    }
}

/* Mark as '.ignore'd each member of F that corresponds to a
   pipe or fifo, and return true if any are not marked.  */
static bool
ignore_fifo_and_pipe (struct File_spec *f, int n_files)
{
  /* When there is no FILE operand and stdin is a pipe or FIFO
     POSIX requires that tail ignore the -f option.
     Since we allow multiple FILE operands, we extend that to say: with -f,
     ignore any "-" operand that corresponds to a pipe or FIFO.  */
  bool some_viable = false;

  for (int i = 0; i < n_files; i++)
    {
      bool is_a_fifo_or_pipe =
        (streq (f[i].name, "-")
         && !f[i].ignore
         && 0 <= f[i].fd
         && (S_ISFIFO (f[i].mode)
             || (HAVE_FIFO_PIPES != 1 && isapipe (f[i].fd))));
      if (is_a_fifo_or_pipe)
        {
          f[i].fd = -1;
          f[i].errnum = -1;
          f[i].ignore = true;
        }
      else
        some_viable = true;
    }

  return some_viable;
}

int
main (int argc, char **argv)
{
  enum header_mode header_mode = multiple_files;
  bool ok = true;
  /* If from_start, the number of items to skip before printing; otherwise,
     the number of items at the end of the file to print.  */
  count_t n_units = DEFAULT_N_LINES;
  int n_files;
  char **file;
  struct File_spec *F;
  bool obsolete_option;

  /* The number of seconds to sleep between iterations.
     During one iteration, every file name or descriptor is checked to
     see if it has changed.  */
  double sleep_interval = 1.0;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  {
    int p = getpagesize ();
    if (IDX_MAX < p)
      xalloc_die ();
    page_size = p;
  }

  have_read_stdin = false;

  count_lines = true;
  forever = from_start = print_headers = false;
  line_end = '\n';
  obsolete_option = parse_obsolete_option (argc, argv, &n_units);
  argc -= obsolete_option;
  argv += obsolete_option;
  parse_options (argc, argv, &n_units, &header_mode, &sleep_interval);

  /* To start printing with item N_UNITS from the start of the file, skip
     N_UNITS - 1 items.  'tail -n +0' is actually meaningless, but for Unix
     compatibility it's treated the same as 'tail -n +1'.  */
  n_units -= from_start && 0 < n_units && n_units < COUNT_MAX;

  if (optind < argc)
    {
      n_files = argc - optind;
      file = argv + optind;
    }
  else
    {
      static char *dummy_stdin = (char *) "-";
      n_files = 1;
      file = &dummy_stdin;
    }

  {
    bool found_hyphen = false;

    for (int i = 0; i < n_files; i++)
      if (streq (file[i], "-"))
        found_hyphen = true;

    /* When following by name, there must be a name.  */
    if (found_hyphen && follow_mode == Follow_name)
      error (EXIT_FAILURE, 0, _("cannot follow %s by name"), quoteaf ("-"));

    /* When following forever, and not using simple blocking, warn if
       any file is '-' as the stats() used to check for input are ineffective.
       This is only a warning, since tail's output (before a failing seek,
       and that from any non-stdin files) might still be useful.  */
    if (forever && found_hyphen)
      {
        struct stat in_stat;
        bool blocking_stdin;
        blocking_stdin = (!nbpids && follow_mode == Follow_descriptor
                          && n_files == 1 && ! fstat (STDIN_FILENO, &in_stat)
                          && ! S_ISREG (in_stat.st_mode));

        if (! blocking_stdin && isatty (STDIN_FILENO))
          error (0, 0, _("warning: following standard input"
                         " indefinitely is ineffective"));
      }
  }

  /* Don't read anything if we'll never output anything.  */
  if (! forever && n_units == (from_start ? COUNT_MAX : 0))
    return EXIT_SUCCESS;

  if (IDX_MAX < n_files)
    xalloc_die ();
  F = xinmalloc (n_files, sizeof *F);
  for (int i = 0; i < n_files; i++)
    {
      F[i].name = file[i];
      F[i].prettyname = streq (file[i], "-") ? _("standard input") : file[i];
    }

  if (header_mode == always
      || (header_mode == multiple_files && n_files > 1))
    print_headers = true;

  xset_binary_mode (STDOUT_FILENO, O_BINARY);

  for (int i = 0; i < n_files; i++)
    ok &= tail_file (&F[i], n_files, n_units);

  if (forever && ignore_fifo_and_pipe (F, n_files))
    {
      /* If stdout is a fifo or pipe, then monitor it
         so that we exit if the reader goes away.  */
      struct stat out_stat;
      if (fstat (STDOUT_FILENO, &out_stat) < 0)
        error (EXIT_FAILURE, errno, _("standard output"));
      monitor_output = (S_ISFIFO (out_stat.st_mode)
                        || (HAVE_FIFO_PIPES != 1 && isapipe (STDOUT_FILENO)));

#if HAVE_INOTIFY
      /* tailable_stdin() checks if the user specifies stdin via  "-",
         or implicitly by providing no arguments. If so, we won't use inotify.
         Technically, on systems with a working /dev/stdin, we *could*,
         but would it be worth it?  Verifying that it's a real device
         and hooked up to stdin is not trivial, while reverting to
         non-inotify-based tail_forever is easy and portable.

         any_remote_file() checks if the user has specified any
         files that reside on remote file systems.  inotify is not used
         in this case because it would miss any updates to the file
         that were not initiated from the local system.

         any_non_remote_file() checks if the user has specified any
         files that don't reside on remote file systems.  inotify is not used
         if there are no open files, as we can't determine if those file
         will be on a remote file system.

         any_symlinks() checks if the user has specified any symbolic links.
         inotify is not used in this case because it returns updated _targets_
         which would not match the specified names.  If we tried to always
         use the target names, then we would miss changes to the symlink itself.

         ok is false when one of the files specified could not be opened for
         reading.  In this case and when following by descriptor,
         tail_forever_inotify() cannot be used (in its current implementation).

         FIXME: inotify doesn't give any notification when a new
         (remote) file or directory is mounted on top a watched file.
         When follow_mode == Follow_name we would ideally like to detect that.
         Note if there is a change to the original file then we'll
         recheck it and follow the new file, or ignore it if the
         file has changed to being remote.

         FIXME-maybe: inotify has a watch descriptor per inode, and hence with
         our current hash implementation will only --follow data for one
         of the names when multiple hardlinked files are specified, or
         for one name when a name is specified multiple times.  */
      if (!disable_inotify && (tailable_stdin (F, n_files)
                               || any_remote_file (F, n_files)
                               || ! any_non_remote_file (F, n_files)
                               || any_symlinks (F, n_files)
                               || any_non_regular_fifo (F, n_files)
                               || (!ok && follow_mode == Follow_descriptor)))
        disable_inotify = true;

      if (!disable_inotify)
        {
          int wd = inotify_init ();
          if (0 <= wd)
            {
              /* Flush any output from tail_file, now, since
                 tail_forever_inotify flushes only after writing,
                 not before reading.  */
              if (fflush (stdout) < 0)
                write_error ();

              Hash_table *ht;
              tail_forever_inotify (wd, F, n_files, sleep_interval, &ht);
              hash_free (ht);
              close (wd);
              errno = 0;
            }
          error (0, errno, _("inotify cannot be used, reverting to polling"));
        }
#endif
      disable_inotify = true;
      tail_forever (F, n_files, sleep_interval);
    }

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, "-");
  main_exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
