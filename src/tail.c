/* tail -- output the last part of file(s)
   Copyright (C) 1989, 90, 91, 1995-2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Can display any amount of data, unlike the Unix version, which uses
   a fixed size buffer and therefore can only deliver a limited number
   of lines.

   Original version by Paul Rubin <phr@ocf.berkeley.edu>.
   Extensions by David MacKenzie <djm@gnu.ai.mit.edu>.
   tail -f for multiple files by Ian Lance Taylor <ian@airs.com>.  */

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>

#include "system.h"
#include "closeout.h"
#include "argmatch.h"
#include "error.h"
#include "inttostr.h"
#include "posixver.h"
#include "safe-read.h"
#include "xnanosleep.h"
#include "xstrtol.h"
#include "xstrtod.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tail"

#define AUTHORS \
  N_ ("Paul Rubin, David MacKenzie, Ian Lance Taylor, and Jim Meyering")

#ifndef ENOSYS
  /* Some systems don't have ENOSYS -- this should be a big enough
     value that no valid errno value will match it.  */
# define ENOSYS 99999
#endif

/* Number of items to tail.  */
#define DEFAULT_N_LINES 10

/* Size of atomic reads.  */
#ifndef BUFSIZ
# define BUFSIZ (512 * 8)
#endif

/* A special value for dump_remainder's N_BYTES parameter.  */
#define COPY_TO_EOF OFF_T_MAX

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
  (S_ISREG (Mode) || S_ISFIFO (Mode) || S_ISCHR (Mode))

static char const *const follow_mode_string[] =
{
  "descriptor", "name", 0
};

static enum Follow_mode const follow_mode_map[] =
{
  Follow_descriptor, Follow_name,
};

struct File_spec
{
  /* The actual file name, or "-" for stdin.  */
  char *name;

  /* File descriptor on which the file is open; -1 if it's not open.  */
  int fd;

  /* The size of the file the last time we checked.  */
  off_t size;

  /* The device and inode of the file the last time we checked.  */
  dev_t dev;
  ino_t ino;

  /* The specified name initially referred to a directory or some other
     type for which tail isn't meaningful.  Unlike for a permission problem
     (tailable, below) once this is set, the name is not checked ever again.  */
  int ignore;

  /* See description of DEFAULT_MAX_N_... below.  */
  unsigned int n_unchanged_stats;

  /* See description of DEFAULT_MAX_N_... below.  */
  unsigned int n_consecutive_size_changes;

  /* A file is tailable if it exists, is readable, and is of type
     IS_TAILABLE_FILE_TYPE.  */
  int tailable;

  /* The value of errno seen last time we checked this file.  */
  int errnum;

};

/* Keep trying to open a file even if it is inaccessible when tail starts
   or if it becomes inaccessible later -- useful only with -f.  */
static int reopen_inaccessible_files;

/* If nonzero, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static int count_lines;

/* Whether we follow the name of each file or the file descriptor
   that is initially associated with each name.  */
static enum Follow_mode follow_mode = Follow_descriptor;

/* If nonzero, read from the ends of all specified files until killed.  */
static int forever;

/* If nonzero, count from start of file instead of end.  */
static int from_start;

/* If nonzero, print filename headers.  */
static int print_headers;

/* When to print the filename banners.  */
enum header_mode
{
  multiple_files, always, never
};

/* When tailing a file by name, if there have been this many consecutive
   iterations for which the size has remained the same, then open/fstat
   the file to determine if that file name is still associated with the
   same device/inode-number pair as before.  This option is meaningful only
   when following by name.  --max-unchanged-stats=N  */
#define DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS 5
static unsigned long max_n_unchanged_stats_between_opens =
  DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS;

/* This variable is used to ensure that a file that is unlinked or moved
   aside, yet always growing will be recognized as having been renamed.
   After detecting this many consecutive size changes for a file, open/fstat
   the file to determine if that file name is still associated with the
   same device/inode-number pair as before.  This option is meaningful only
   when following by name.  --max-consecutive-size-changes=N  */
#define DEFAULT_MAX_N_CONSECUTIVE_SIZE_CHANGES 200
static unsigned long max_n_consecutive_size_changes_between_opens =
  DEFAULT_MAX_N_CONSECUTIVE_SIZE_CHANGES;

/* The name this program was run with.  */
char *program_name;

/* The process ID of the process (presumably on the current host)
   that is writing to all followed files.  */
static pid_t pid;

/* Nonzero if we have ever read standard input.  */
static int have_read_stdin;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  RETRY_OPTION = CHAR_MAX + 1,
  MAX_UNCHANGED_STATS_OPTION,

  /* FIXME: remove this in 2001, unless someone can show a good
     reason to keep it.  */
  MAX_CONSECUTIVE_SIZE_CHANGES_OPTION,

  PID_OPTION,
  LONG_FOLLOW_OPTION
};

static struct option const long_options[] =
{
  /* --allow-missing is deprecated; use --retry instead
     FIXME: remove it some day */
  {"allow-missing", no_argument, NULL, RETRY_OPTION},
  {"bytes", required_argument, NULL, 'c'},
  {"follow", optional_argument, NULL, LONG_FOLLOW_OPTION},
  {"lines", required_argument, NULL, 'n'},
  {"max-unchanged-stats", required_argument, NULL, MAX_UNCHANGED_STATS_OPTION},
  {"max-consecutive-size-changes", required_argument, NULL,
   MAX_CONSECUTIVE_SIZE_CHANGES_OPTION},
  {"pid", required_argument, NULL, PID_OPTION},
  {"quiet", no_argument, NULL, 'q'},
  {"retry", no_argument, NULL, RETRY_OPTION},
  {"silent", no_argument, NULL, 'q'},
  {"sleep-interval", required_argument, NULL, 's'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"),
	      program_name);
      printf (_("\
Print the last %d lines of each FILE to standard output.\n\
With more than one FILE, precede each with a header giving the file name.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), DEFAULT_N_LINES);
     fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
     fputs (_("\
      --retry              keep trying to open a file even if it is\n\
                           inaccessible when tail starts or if it becomes\n\
                           inaccessible later -- useful only with -f\n\
  -c, --bytes=N            output the last N bytes\n\
"), stdout);
     fputs (_("\
  -f, --follow[={name|descriptor}]\n\
                           output appended data as the file grows;\n\
                           -f, --follow, and --follow=descriptor are\n\
                           equivalent\n\
  -F                       same as --follow=name --retry\n\
"), stdout);
     printf (_("\
  -n, --lines=N            output the last N lines, instead of the last %d\n\
      --max-unchanged-stats=N\n\
                           with --follow=name, reopen a FILE which has not\n\
                           changed size after N (default %d) iterations\n\
                           to see if it has been unlinked or renamed\n\
                           (this is the usual case of rotated log files)\n\
"),
	     DEFAULT_N_LINES,
	     DEFAULT_MAX_N_UNCHANGED_STATS_BETWEEN_OPENS
	     );
     fputs (_("\
      --pid=PID            with -f, terminate after process ID, PID dies\n\
  -q, --quiet, --silent    never output headers giving file names\n\
  -s, --sleep-interval=S   with -f, sleep for approximately S seconds\n\
                           (default 1.0) between iterations.\n\
  -v, --verbose            always output headers giving file names\n\
"), stdout);
     fputs (HELP_OPTION_DESCRIPTION, stdout);
     fputs (VERSION_OPTION_DESCRIPTION, stdout);
     fputs (_("\
\n\
If the first character of N (the number of bytes or lines) is a `+',\n\
print beginning with the Nth item from the start of each file, otherwise,\n\
print the last N items in the file.  N may have a multiplier suffix:\n\
b for 512, k for 1024, m for 1048576 (1 Meg).\n\
\n\
"), stdout);
     fputs (_("\
With --follow (-f), tail defaults to following the file descriptor, which\n\
means that even if a tail'ed file is renamed, tail will continue to track\n\
its end.  \
"), stdout);
     fputs (_("\
This default behavior is not desirable when you really want to\n\
track the actual name of the file, not the file descriptor (e.g., log\n\
rotation).  Use --follow=name in that case.  That causes tail to track the\n\
named file by reopening it periodically to see if it has been removed and\n\
recreated by some other program.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static int
valid_file_spec (struct File_spec const *f)
{
  /* Exactly one of the following subexpressions must be true. */
  return ((f->fd == -1) ^ (f->errnum == 0));
}

static char *
pretty_name (struct File_spec const *f)
{
  return (STREQ (f->name, "-") ? "standard input" : f->name);
}

static void
xwrite (int fd, char const *buffer, size_t n_bytes)
{
  assert (fd == STDOUT_FILENO);
  if (n_bytes > 0 && fwrite (buffer, 1, n_bytes, stdout) == 0)
    error (EXIT_FAILURE, errno, _("write error"));
}

static void
close_fd (int fd, const char *filename)
{
  if (fd != -1 && fd != STDIN_FILENO && close (fd))
    {
      error (0, errno, _("closing %s (fd=%d)"), filename, fd);
    }
}

static void
write_header (const char *pretty_filename)
{
  static int first_file = 1;

  printf ("%s==> %s <==\n", (first_file ? "" : "\n"), pretty_filename);
  first_file = 0;
}

/* Read and output N_BYTES of file PRETTY_FILENAME starting at the current
   position in FD.  If N_BYTES is COPY_TO_EOF, then copy until end of file.
   Return the number of bytes read from the file.  */

static long
dump_remainder (const char *pretty_filename, int fd, off_t n_bytes)
{
  char buffer[BUFSIZ];
  size_t bytes_read;
  long n_written;
  off_t n_remaining = n_bytes;

  n_written = 0;
  while (1)
    {
      long n = MIN (n_remaining, (off_t) BUFSIZ);
      bytes_read = safe_read (fd, buffer, n);
      if (bytes_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, "%s", pretty_filename);
      if (bytes_read == 0)
	break;
      xwrite (STDOUT_FILENO, buffer, bytes_read);
      n_remaining -= bytes_read;
      n_written += bytes_read;
    }

  return n_written;
}

/* Call lseek with the specified arguments, where file descriptor FD
   corresponds to the file, FILENAME.
   Give a diagnostic and exit nonzero if lseek fails.  */

static void
xlseek (int fd, off_t offset, int whence, char const *filename)
{
  off_t new_offset = lseek (fd, offset, whence);
  char buf[INT_BUFSIZE_BOUND (off_t)];
  char *s;

  if (0 <= new_offset)
    return;

  s = offtostr (offset, buf);
  switch (whence)
    {
    case SEEK_SET:
      error (EXIT_FAILURE, errno, _("%s: cannot seek to offset %s"),
	     filename, s);
      break;
    case SEEK_CUR:
      error (EXIT_FAILURE, errno, _("%s: cannot seek to relative offset %s"),
	     filename, s);
      break;
    case SEEK_END:
      error (EXIT_FAILURE, errno,
	     _("%s: cannot seek to end-relative offset %s"),
	     filename, s);
      break;
    default:
      abort ();
    }
}

/* Print the last N_LINES lines from the end of file FD.
   Go backward through the file, reading `BUFSIZ' bytes at a time (except
   probably the first), until we hit the start of the file or have
   read NUMBER newlines.
   START_POS is the starting position of the read pointer for the file
   associated with FD (may be nonzero).
   FILE_LENGTH is the length of the file (one more than the offset of the
   last byte of the file).
   Return 0 if successful, 1 if an error occurred.  */

static int
file_lines (const char *pretty_filename, int fd, long int n_lines,
	    off_t start_pos, off_t file_length)
{
  char buffer[BUFSIZ];
  size_t bytes_read;
  off_t pos = file_length;

  if (n_lines == 0)
    return 0;

  /* Set `bytes_read' to the size of the last, probably partial, buffer;
     0 < `bytes_read' <= `BUFSIZ'.  */
  bytes_read = (pos - start_pos) % BUFSIZ;
  if (bytes_read == 0)
    bytes_read = BUFSIZ;
  /* Make `pos' a multiple of `BUFSIZ' (0 if the file is short), so that all
     reads will be on block boundaries, which might increase efficiency.  */
  pos -= bytes_read;
  xlseek (fd, pos, SEEK_SET, pretty_filename);
  bytes_read = safe_read (fd, buffer, bytes_read);
  if (bytes_read == SAFE_READ_ERROR)
    {
      error (0, errno, "%s", pretty_filename);
      return 1;
    }

  /* Count the incomplete line on files that don't end with a newline.  */
  if (bytes_read && buffer[bytes_read - 1] != '\n')
    --n_lines;

  do
    {
      /* Scan backward, counting the newlines in this bufferfull.  */

      size_t n = bytes_read;
      while (n)
	{
	  char const *nl;
	  nl = memrchr (buffer, '\n', n);
	  if (nl == NULL)
	    break;
	  n = nl - buffer;
	  if (n_lines-- == 0)
	    {
	      /* If this newline isn't the last character in the buffer,
	         output the part that is after it.  */
	      if (n != bytes_read - 1)
		xwrite (STDOUT_FILENO, nl + 1, bytes_read - (n + 1));
	      dump_remainder (pretty_filename, fd,
			      file_length - (pos + bytes_read));
	      return 0;
	    }
	}

      /* Not enough newlines in that bufferfull.  */
      if (pos == start_pos)
	{
	  /* Not enough lines in the file; print the entire file.  */
	  xlseek (fd, start_pos, SEEK_SET, pretty_filename);
	  dump_remainder (pretty_filename, fd, file_length);
	  return 0;
	}
      pos -= BUFSIZ;
      xlseek (fd, pos, SEEK_SET, pretty_filename);

      bytes_read = safe_read (fd, buffer, BUFSIZ);
      if (bytes_read == SAFE_READ_ERROR)
	{
	  error (0, errno, "%s", pretty_filename);
	  return 1;
	}
    }
  while (bytes_read > 0);

  return 0;
}

/* Print the last N_LINES lines from the end of the standard input,
   open for reading as pipe FD.
   Buffer the text as a linked list of LBUFFERs, adding them as needed.
   Return 0 if successful, 1 if an error occured.  */

static int
pipe_lines (const char *pretty_filename, int fd, long int n_lines)
{
  struct linebuffer
  {
    int nbytes, nlines;
    char buffer[BUFSIZ];
    struct linebuffer *next;
  };
  typedef struct linebuffer LBUFFER;
  LBUFFER *first, *last, *tmp;
  int i;			/* Index into buffers.  */
  int total_lines = 0;		/* Total number of newlines in all buffers.  */
  int errors = 0;
  int nbytes;			/* Size of most recent read */

  first = last = (LBUFFER *) xmalloc (sizeof (LBUFFER));
  first->nbytes = first->nlines = 0;
  first->next = NULL;
  tmp = (LBUFFER *) xmalloc (sizeof (LBUFFER));

  /* Input is always read into a fresh buffer.  */
  while ((nbytes = tmp->nbytes = safe_read (fd, tmp->buffer, BUFSIZ)) > 0)
    {
      tmp->nlines = 0;
      tmp->next = NULL;

      /* Count the number of newlines just read.  */
      for (i = 0; i < tmp->nbytes; i++)
	if (tmp->buffer[i] == '\n')
	  ++tmp->nlines;
      total_lines += tmp->nlines;

      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, `nbytes' can
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
	    tmp = (LBUFFER *) xmalloc (sizeof (LBUFFER));
	}
    }

  free ((char *) tmp);

  if (nbytes < 0)
    {
      error (0, errno, "%s", pretty_filename);
      errors = 1;
      goto free_lbuffers;
    }

  /* If the file is empty, then bail out.  */
  if (last->nbytes == 0)
    goto free_lbuffers;

  /* This prevents a core dump when the pipe contains no newlines.  */
  if (n_lines == 0)
    goto free_lbuffers;

  /* Count the incomplete line on files that don't end with a newline.  */
  if (last->buffer[last->nbytes - 1] != '\n')
    {
      ++last->nlines;
      ++total_lines;
    }

  /* Run through the list, printing lines.  First, skip over unneeded
     buffers.  */
  for (tmp = first; total_lines - tmp->nlines > n_lines; tmp = tmp->next)
    total_lines -= tmp->nlines;

  /* Find the correct beginning, then print the rest of the file.  */
  if (total_lines > n_lines)
    {
      const char *cp;

      /* Skip `total_lines' - `n_lines' newlines.  We made sure that
         `total_lines' - `n_lines' <= `tmp->nlines'.  */
      cp = tmp->buffer;
      for (i = total_lines - n_lines; i; --i)
	while (*cp++ != '\n')
	  /* Do nothing.  */ ;
      i = cp - tmp->buffer;
    }
  else
    i = 0;
  xwrite (STDOUT_FILENO, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite (STDOUT_FILENO, tmp->buffer, tmp->nbytes);

free_lbuffers:
  while (first)
    {
      tmp = first->next;
      free ((char *) first);
      first = tmp;
    }
  return errors;
}

/* Print the last N_BYTES characters from the end of pipe FD.
   This is a stripped down version of pipe_lines.
   Return 0 if successful, 1 if an error occurred.  */

static int
pipe_bytes (const char *pretty_filename, int fd, off_t n_bytes)
{
  struct charbuffer
  {
    int nbytes;
    char buffer[BUFSIZ];
    struct charbuffer *next;
  };
  typedef struct charbuffer CBUFFER;
  CBUFFER *first, *last, *tmp;
  int i;			/* Index into buffers.  */
  int total_bytes = 0;		/* Total characters in all buffers.  */
  int errors = 0;

  first = last = (CBUFFER *) xmalloc (sizeof (CBUFFER));
  first->nbytes = 0;
  first->next = NULL;
  tmp = (CBUFFER *) xmalloc (sizeof (CBUFFER));

  /* Input is always read into a fresh buffer.  */
  while ((tmp->nbytes = safe_read (fd, tmp->buffer, BUFSIZ)) > 0)
    {
      tmp->next = NULL;

      total_bytes += tmp->nbytes;
      /* If there is enough room in the last buffer read, just append the new
         one to it.  This is because when reading from a pipe, `nbytes' can
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
	      tmp = (CBUFFER *) xmalloc (sizeof (CBUFFER));
	    }
	}
    }
  if (tmp->nbytes == -1)
    {
      error (0, errno, "%s", pretty_filename);
      errors = 1;
      free ((char *) tmp);
      goto free_cbuffers;
    }

  free ((char *) tmp);

  /* Run through the list, printing characters.  First, skip over unneeded
     buffers.  */
  for (tmp = first; total_bytes - tmp->nbytes > n_bytes; tmp = tmp->next)
    total_bytes -= tmp->nbytes;

  /* Find the correct beginning, then print the rest of the file.
     We made sure that `total_bytes' - `n_bytes' <= `tmp->nbytes'.  */
  if (total_bytes > n_bytes)
    i = total_bytes - n_bytes;
  else
    i = 0;
  xwrite (STDOUT_FILENO, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    xwrite (STDOUT_FILENO, tmp->buffer, tmp->nbytes);

free_cbuffers:
  while (first)
    {
      tmp = first->next;
      free ((char *) first);
      first = tmp;
    }
  return errors;
}

/* Skip N_BYTES characters from the start of pipe FD, and print
   any extra characters that were read beyond that.
   Return 1 on error, 0 if ok, -1 if EOF.  */

static int
start_bytes (const char *pretty_filename, int fd, off_t n_bytes)
{
  char buffer[BUFSIZ];
  size_t bytes_read = 0;

  while (0 < n_bytes)
    {
      bytes_read = safe_read (fd, buffer, BUFSIZ);
      if (bytes_read == 0)
	return -1;
      if (bytes_read == SAFE_READ_ERROR)
	{
	  error (0, errno, "%s", pretty_filename);
	  return 1;
	}
      n_bytes -= bytes_read;
    }

  if (n_bytes < 0)
    xwrite (STDOUT_FILENO, &buffer[bytes_read + n_bytes], -n_bytes);

  return 0;
}

/* Skip N_LINES lines at the start of file or pipe FD, and print
   any extra characters that were read beyond that.
   Return 1 on error, 0 if ok, -1 if EOF.  */

static int
start_lines (const char *pretty_filename, int fd, long int n_lines)
{
  char buffer[BUFSIZ];
  size_t bytes_read = 0;
  size_t bytes_to_skip = 0;

  if (n_lines == 0)
    return 0;

  while (n_lines)
    {
      bytes_to_skip = 0;
      bytes_read = safe_read (fd, buffer, BUFSIZ);
      if (bytes_read == 0) /* EOF */
	return -1;
      if (bytes_read == SAFE_READ_ERROR) /* error */
	{
	  error (0, errno, "%s", pretty_filename);
	  return 1;
	}

      while (bytes_to_skip < bytes_read)
	if (buffer[bytes_to_skip++] == '\n' && --n_lines == 0)
	  break;
    }

  if (bytes_to_skip < bytes_read)
    {
      xwrite (STDOUT_FILENO, &buffer[bytes_to_skip],
	      bytes_read - bytes_to_skip);
    }

  return 0;
}

/* FIXME: describe */

static void
recheck (struct File_spec *f)
{
  /* open/fstat the file and announce if dev/ino have changed */
  struct stat new_stats;
  int fd;
  int fail = 0;
  int is_stdin = (STREQ (f->name, "-"));
  int was_tailable = f->tailable;
  int prev_errnum = f->errnum;
  int new_file;

  assert (valid_file_spec (f));

  fd = (is_stdin ? STDIN_FILENO : open (f->name, O_RDONLY));

  /* If the open fails because the file doesn't exist,
     then mark the file as not tailable.  */
  f->tailable = !(reopen_inaccessible_files && fd == -1);

  if (fd == -1 || fstat (fd, &new_stats) < 0)
    {
      fail = 1;
      f->errnum = errno;
      if (!f->tailable)
	{
	  if (was_tailable)
	    {
	      /* FIXME-maybe: detect the case in which the file first becomes
		 unreadable (perms), and later becomes readable again and can
		 be seen to be the same file (dev/ino).  Otherwise, tail prints
		 the entire contents of the file when it becomes readable.  */
	      error (0, f->errnum, _("`%s' has become inaccessible"),
		     pretty_name (f));
	    }
	  else
	    {
	      /* say nothing... it's still not tailable */
	    }
	}
      else if (prev_errnum != errno)
	{
	  error (0, errno, "%s", pretty_name (f));
	}
    }
  else if (!IS_TAILABLE_FILE_TYPE (new_stats.st_mode))
    {
      fail = 1;
      f->errnum = -1;
      error (0, 0, _("`%s' has been replaced with an untailable file;\
 giving up on this name"),
	     pretty_name (f));
      f->ignore = 1;
    }
  else
    {
      f->errnum = 0;
    }

  new_file = 0;
  if (fail)
    {
      close_fd (fd, pretty_name (f));
      close_fd (f->fd, pretty_name (f));
      f->fd = -1;
    }
  else if (prev_errnum && prev_errnum != ENOENT)
    {
      new_file = 1;
      assert (f->fd == -1);
      error (0, 0, _("`%s' has become accessible"), pretty_name (f));
    }
  else if (f->ino != new_stats.st_ino || f->dev != new_stats.st_dev)
    {
      new_file = 1;
      if (f->fd == -1)
	{
	  error (0, 0,
		 _("`%s' has appeared;  following end of new file"),
		 pretty_name (f));
	}
      else
	{
	  /* Close the old one.  */
	  close_fd (f->fd, pretty_name (f));

	  /* File has been replaced (e.g., via log rotation) --
	     tail the new one.  */
	  error (0, 0,
		 _("`%s' has been replaced;  following end of new file"),
		 pretty_name (f));
	}
    }
  else
    {
      if (f->fd == -1)
	{
	  /* This happens when one iteration finds the file missing,
	     then the preceding <dev,inode> pair is reused as the
	     file is recreated.  */
	  new_file = 1;
	}
      else
	{
	  close_fd (fd, pretty_name (f));
	}
    }

  if (new_file)
    {
      /* Record new file info in f.  */
      f->fd = fd;
      f->size = 0; /* Start at the beginning of the file...  */
      f->dev = new_stats.st_dev;
      f->ino = new_stats.st_ino;
      f->n_unchanged_stats = 0;
      f->n_consecutive_size_changes = 0;
      f->ignore = 0;
      xlseek (f->fd, f->size, SEEK_SET, pretty_name (f));
    }
}

/* FIXME: describe */

static unsigned int
n_live_files (const struct File_spec *f, int n_files)
{
  int i;
  unsigned int n_live = 0;

  for (i = 0; i < n_files; i++)
    {
      if (f[i].fd >= 0)
	++n_live;
    }
  return n_live;
}

/* Tail NFILES files forever, or until killed.
   The pertinent information for each file is stored in an entry of F.
   Loop over each of them, doing an fstat to see if they have changed size,
   and an occasional open/fstat to see if any dev/ino pair has changed.
   If none of them have changed size in one iteration, sleep for a
   while and try again.  Continue until the user interrupts us.  */

static void
tail_forever (struct File_spec *f, int nfiles, double sleep_interval)
{
  int last;
  int writer_is_dead = 0;

  last = nfiles - 1;

  while (1)
    {
      int i;
      int any_changed;

      any_changed = 0;
      for (i = 0; i < nfiles; i++)
	{
	  struct stat stats;

	  if (f[i].ignore)
	    continue;

	  if (f[i].fd < 0)
	    {
	      recheck (&f[i]);
	      continue;
	    }

	  if (fstat (f[i].fd, &stats) < 0)
	    {
	      f[i].fd = -1;
	      f[i].errnum = errno;
	      error (0, errno, "%s", pretty_name (&f[i]));
	      continue;
	    }

	  if (stats.st_size == f[i].size)
	    {
	      f[i].n_consecutive_size_changes = 0;
	      if ((max_n_unchanged_stats_between_opens
		   <= f[i].n_unchanged_stats++)
		  && follow_mode == Follow_name)
		{
		  recheck (&f[i]);
		  f[i].n_unchanged_stats = 0;
		}
	      continue;
	    }

	  /* Ensure that a file that's unlinked or moved aside, yet always
	     growing will be recognized as having been renamed.  */
	  if ((max_n_consecutive_size_changes_between_opens
	       <= f[i].n_consecutive_size_changes++)
	      && follow_mode == Follow_name)
	    {
	      f[i].n_consecutive_size_changes = 0;
	      recheck (&f[i]);
	      continue;
	    }

	  /* This file has changed size.  Print out what we can, and
	     then keep looping.  */

	  any_changed = 1;

	  /* reset counter */
	  f[i].n_unchanged_stats = 0;

	  if (stats.st_size < f[i].size)
	    {
	      error (0, 0, _("%s: file truncated"), pretty_name (&f[i]));
	      last = i;
	      xlseek (f[i].fd, (off_t) stats.st_size, SEEK_SET,
		      pretty_name (&f[i]));
	      f[i].size = stats.st_size;
	      continue;
	    }

	  if (i != last)
	    {
	      if (print_headers)
		write_header (pretty_name (&f[i]));
	      last = i;
	    }
	  f[i].size += dump_remainder (pretty_name (&f[i]), f[i].fd,
				       COPY_TO_EOF);
	}

      if (n_live_files (f, nfiles) == 0 && ! reopen_inaccessible_files)
	{
	  error (0, 0, _("no files remaining"));
	  break;
	}

      /* If none of the files changed size, sleep.  */
      if (!any_changed)
	{
	  if (writer_is_dead)
	    break;

	  if (xnanosleep (sleep_interval))
	    error (EXIT_FAILURE, errno, _("cannot read realtime clock"));

	  /* Once the writer is dead, read the files once more to
	     avoid a race condition.  */
	  writer_is_dead = (pid != 0
			    && kill (pid, 0) != 0
			    /* Handle the case in which you cannot send a
			       signal to the writer, so kill fails and sets
			       errno to EPERM.  */
			    && errno != EPERM);
	}
    }
}

/* Output the last N_BYTES bytes of file FILENAME open for reading in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_bytes (const char *pretty_filename, int fd, off_t n_bytes)
{
  struct stat stats;

  /* We need binary input, since `tail' relies on `lseek' and byte counts,
     while binary output will preserve the style (Unix/DOS) of text file.  */
  SET_BINARY2 (fd, STDOUT_FILENO);

  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", pretty_filename);
      return 1;
    }

  if (from_start)
    {
      if (S_ISREG (stats.st_mode))
	{
	  xlseek (fd, n_bytes, SEEK_CUR, pretty_filename);
	}
      else
	{
	  int t;
	  if ((t = start_bytes (pretty_filename, fd, n_bytes)) < 0)
	    return 0;
	  if (t)
	    return 1;
	}
      dump_remainder (pretty_filename, fd, COPY_TO_EOF);
    }
  else
    {
      if (S_ISREG (stats.st_mode))
	{
	  off_t current_pos, end_pos;
	  off_t bytes_remaining;

	  if ((current_pos = lseek (fd, (off_t) 0, SEEK_CUR)) != -1
	      && (end_pos = lseek (fd, (off_t) 0, SEEK_END)) != -1)
	    {
	      off_t diff;
	      /* Be careful here.  The current position may actually be
		 beyond the end of the file.  */
	      bytes_remaining = (diff = end_pos - current_pos) < 0 ? 0 : diff;
	    }
	  else
	    {
	      error (0, errno, "%s", pretty_filename);
	      return 1;
	    }

	  if (bytes_remaining <= n_bytes)
	    {
	      /* From the current position to end of file, there are no
		 more bytes than have been requested.  So reposition the
		 file pointer to the incoming current position and print
		 everything after that.  */
	      xlseek (fd, current_pos, SEEK_SET, pretty_filename);
	    }
	  else
	    {
	      /* There are more bytes remaining than were requested.
		 Back up.  */
	      xlseek (fd, -n_bytes, SEEK_END, pretty_filename);
	    }
	  dump_remainder (pretty_filename, fd, n_bytes);
	}
      else
	return pipe_bytes (pretty_filename, fd, n_bytes);
    }
  return 0;
}

/* Output the last N_LINES lines of file FILENAME open for reading in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_lines (const char *pretty_filename, int fd, long int n_lines)
{
  struct stat stats;

  /* We need binary input, since `tail' relies on `lseek' and byte counts,
     while binary output will preserve the style (Unix/DOS) of text file.  */
  SET_BINARY2 (fd, STDOUT_FILENO);

  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", pretty_filename);
      return 1;
    }

  if (from_start)
    {
      int t;
      if ((t = start_lines (pretty_filename, fd, n_lines)) < 0)
	return 0;
      if (t)
	return 1;
      dump_remainder (pretty_filename, fd, COPY_TO_EOF);
    }
  else
    {
      off_t length;
      off_t start_pos;

      /* Use file_lines only if FD refers to a regular file for
	 which lseek (... SEEK_END) works.  */
      if (S_ISREG (stats.st_mode)
	  && (start_pos = lseek (fd, (off_t) 0, SEEK_CUR)) != -1
	  && start_pos < (length = lseek (fd, (off_t) 0, SEEK_END)))
	{
	  if (length != 0 && file_lines (pretty_filename, fd, n_lines,
					 start_pos, length))
	    return 1;
	}
      else
	return pipe_lines (pretty_filename, fd, n_lines);
    }
  return 0;
}

/* Display the last N_UNITS units of file FILENAME, open for reading
   in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail (const char *pretty_filename, int fd, off_t n_units)
{
  if (count_lines)
    return tail_lines (pretty_filename, fd, (long) n_units);
  else
    return tail_bytes (pretty_filename, fd, n_units);
}

/* Display the last N_UNITS units of the file described by F.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_file (struct File_spec *f, off_t n_units)
{
  int fd, errors;

  int is_stdin = (STREQ (f->name, "-"));

  if (is_stdin)
    {
      have_read_stdin = 1;
      fd = STDIN_FILENO;
    }
  else
    {
      fd = open (f->name, O_RDONLY);
    }

  f->tailable = !(reopen_inaccessible_files && fd == -1);

  if (fd == -1)
    {
      if (forever)
	{
	  f->fd = -1;
	  f->errnum = errno;
	  f->ignore = 0;
	  f->ino = 0;
	  f->dev = 0;
	}
      error (0, errno, "%s", pretty_name (f));
      errors = 1;
    }
  else
    {
      if (print_headers)
	write_header (pretty_name (f));
      errors = tail (pretty_name (f), fd, n_units);
      if (forever)
	{
	  struct stat stats;

	  f->errnum = 0;
	  if (fstat (fd, &stats) < 0)
	    {
	      errors = 1;
	      f->errnum = errno;
	      error (0, errno, "%s", pretty_name (f));
	    }
	  else if (!IS_TAILABLE_FILE_TYPE (stats.st_mode))
	    {
	      error (0, 0, _("%s: cannot follow end of this type of file;\
 giving up on this name"),
		     pretty_name (f));
	      errors = 1;
	      f->errnum = -1;
	      f->ignore = 1;
	    }

	  if (errors)
	    {
	      close_fd (fd, pretty_name (f));
	      f->fd = -1;
	    }
	  else
	    {
	      f->fd = fd;
	      f->size = stats.st_size;
	      f->dev = stats.st_dev;
	      f->ino = stats.st_ino;
	      f->n_unchanged_stats = 0;
	      f->n_consecutive_size_changes = 0;
	      f->ignore = 0;
	    }
	}
      else
	{
	  if (!is_stdin && close (fd))
	    {
	      error (0, errno, "%s", pretty_name (f));
	      errors = 1;
	    }
	}
    }

  return errors;
}

/* If the command line arguments are of the obsolescent form and the
   option string is well-formed, set *FAIL to zero, set *N_UNITS, the
   globals COUNT_LINES, FOREVER, and FROM_START, and return non-zero.
   Otherwise, if the command line arguments appear to be of the
   obsolescent form but the option string is malformed, set *FAIL to
   non-zero, don't modify any other parameter or global variable, and
   return non-zero. Otherwise, return zero and don't modify any parameter
   or global variable.  */

static int
parse_obsolescent_option (int argc, const char *const *argv,
			  off_t *n_units, int *fail)
{
  const char *p = argv[1];
  const char *n_string = NULL;
  const char *n_string_end;
  bool obsolete_usage;

  int t_from_start;
  int t_count_lines;
  int t_forever;

  /* With the obsolescent form, there is one option string and
     (technically) at most one file argument.  But we allow two or more
     by default.  */
  if (argc < 2)
    return 0;

  obsolete_usage = (posix2_version () < 200112);

  /* If P starts with `+' and the POSIX version predates 1003.1-2001,
     or if P starts with `-N' (where N is a digit), or `-l', then it
     is obsolescent.  Return zero otherwise.  */
  if (! ((p[0] == '+' && obsolete_usage)
	 || (p[0] == '-' && (p[1] == 'l' || ISDIGIT (p[1])))))
    return 0;

  if (*p == '+')
    t_from_start = 1;
  else if (*p == '-')
    t_from_start = 0;
  else
    return 0;

  ++p;
  if (ISDIGIT (*p))
    {
      n_string = p;
      do
	{
	  ++p;
	}
      while (ISDIGIT (*p));
    }
  n_string_end = p;

  t_count_lines = 1;
  if (*p == 'c' || *p == 'b')
    {
      t_count_lines = 0;
      ++p;
    }
  else if (*p == 'l')
    {
      ++p;
    }

  t_forever = 0;
  if (*p == 'f')
    {
      t_forever = 1;
      ++p;
    }

  if (*p != '\0')
    {
      /* If (argv[1] begins with a `+' or if it begins with `-' followed
	 by a digit), but has an invalid suffix character, give a diagnostic
	 and indicate to caller that this *is* of the obsolescent form,
	 but that it's an invalid option.  */
      if (t_from_start || n_string)
	{
	  error (0, 0,
		 _("%c: invalid suffix character in obsolescent option" ), *p);
	  *fail = 1;
	  return 1;
	}

      /* Otherwise, it might be a valid non-obsolescent option like -n.  */
      return 0;
    }

  *fail = 0;
  if (n_string == NULL)
    *n_units = DEFAULT_N_LINES;
  else
    {
      strtol_error s_err;
      uintmax_t tmp;
      char *end;

      s_err = xstrtoumax (n_string, &end, 10, &tmp,
			  *n_string_end == 'b' ? "b" : NULL);
      if (s_err == LONGINT_OK && tmp <= OFF_T_MAX)
	*n_units = (off_t) tmp;
      else
	{
	  /* Extract a NUL-terminated string for the error message.  */
	  size_t len = n_string_end - n_string;
	  char *n_string_tmp = xmalloc (len + 1);

	  strncpy (n_string_tmp, n_string, len);
	  n_string_tmp[len] = '\0';

	  error (0, 0,
		 _("%s: %s is so large that it is not representable"),
		 n_string_tmp, (t_count_lines
				? _("number of lines")
				: _("number of bytes")));
	  free (n_string_tmp);
	  *fail = 1;
	}
    }

  if (!*fail)
    {
      if (argc > 3)
	{
	  /* When POSIXLY_CORRECT is set, enforce the `at most one
	     file argument' requirement.  */
	  if (getenv ("POSIXLY_CORRECT"))
	    {
	      error (0, 0, _("\
too many arguments;  When using tail's obsolescent option syntax (%s)\n\
there may be no more than one file argument.  Use the equivalent -n or -c\n\
option instead."), argv[1]);
	      *fail = 1;
	      return 1;
	    }

#if DISABLED  /* FIXME: enable or remove this warning.  */
	  error (0, 0, _("\
Warning: it is not portable to use two or more file arguments with\n\
tail's obsolescent option syntax (%s).  Use the equivalent -n or -c\n\
option instead."), argv[1]);
#endif
	}

      if (! obsolete_usage)
	{
	  error (0, 0, _("`%s' option is obsolete; use `%s-%c %.*s'"),
		 argv[1], t_forever ? " -f" : "", t_count_lines ? 'n' : 'c',
		 (int) (n_string_end - n_string), n_string);
	  usage (EXIT_FAILURE);
	}

      /* Set globals.  */
      from_start = t_from_start;
      count_lines = t_count_lines;
      forever = t_forever;
    }

  return 1;
}

static void
parse_options (int argc, char **argv,
	       off_t *n_units, enum header_mode *header_mode,
	       double *sleep_interval)
{
  int c;

  count_lines = 1;
  forever = from_start = print_headers = 0;

  while ((c = getopt_long (argc, argv, "c:n:fFqs:v", long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'F':
	  forever = 1;
	  follow_mode = Follow_name;
	  reopen_inaccessible_files = 1;
	  break;

	case 'c':
	case 'n':
	  count_lines = (c == 'n');
	  if (*optarg == '+')
	    from_start = 1;
	  else if (*optarg == '-')
	    ++optarg;

	  {
	    strtol_error s_err;
	    uintmax_t n;
	    s_err = xstrtoumax (optarg, NULL, 10, &n, "bkm");
	    if (s_err != LONGINT_OK)
	      {
		error (EXIT_FAILURE, 0, "%s: %s", optarg,
		       (c == 'n'
			? _("invalid number of lines")
			: _("invalid number of bytes")));
	      }

	    if (OFF_T_MAX < n)
	      error (EXIT_FAILURE, 0,
		   _("%s is larger than the maximum file size on this system"),
		     optarg);
	    *n_units = (off_t) n;
	  }
	  break;

	case 'f':
	case LONG_FOLLOW_OPTION:
	  forever = 1;
	  if (optarg == NULL)
	    follow_mode = DEFAULT_FOLLOW_MODE;
	  else
	    follow_mode = XARGMATCH ("--follow", optarg,
				     follow_mode_string, follow_mode_map);
	  break;

	case RETRY_OPTION:
	  reopen_inaccessible_files = 1;
	  break;

	case MAX_UNCHANGED_STATS_OPTION:
	  /* --max-unchanged-stats=N */
	  if (xstrtoul (optarg, NULL, 10,
			&max_n_unchanged_stats_between_opens, "") != LONGINT_OK)
	    {
	      error (EXIT_FAILURE, 0,
	       _("%s: invalid maximum number of unchanged stats between opens"),
		     optarg);
	    }
	  break;

	case MAX_CONSECUTIVE_SIZE_CHANGES_OPTION:
  	  /* --max-consecutive-size-changes=N */
	  if (xstrtoul (optarg, NULL, 10,
			&max_n_consecutive_size_changes_between_opens, "")
	      != LONGINT_OK)
	    {
	      error (EXIT_FAILURE, 0,
		   _("%s: invalid maximum number of consecutive size changes"),
		     optarg);
	    }
	  break;

	case PID_OPTION:
	  {
	    strtol_error s_err;
	    unsigned long int tmp_ulong;
	    s_err = xstrtoul (optarg, NULL, 10, &tmp_ulong, "");
	    if (s_err != LONGINT_OK || tmp_ulong > PID_T_MAX)
	      {
		error (EXIT_FAILURE, 0, _("%s: invalid PID"), optarg);
	      }
	    pid = tmp_ulong;
	  }
	  break;

	case 'q':
	  *header_mode = never;
	  break;

	case 's':
	  {
	    double s;
	    if (xstrtod (optarg, NULL, &s) || ! (0 <= s))
	      error (EXIT_FAILURE, 0,
		     _("%s: invalid number of seconds"), optarg);
	    *sleep_interval = s;
	  }
	  break;

	case 'v':
	  *header_mode = always;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (reopen_inaccessible_files && follow_mode != Follow_name)
    error (0, 0, _("warning: --retry is useful only when following by name"));

  if (pid && !forever)
    error (0, 0,
	   _("warning: PID ignored; --pid=PID is useful only when following"));
  else if (pid && kill (pid, 0) != 0 && errno == ENOSYS)
    {
      error (0, 0, _("warning: --pid=PID is not supported on this system"));
      pid = 0;
    }
}

int
main (int argc, char **argv)
{
  enum header_mode header_mode = multiple_files;
  int exit_status = 0;
  /* If from_start, the number of items to skip before printing; otherwise,
     the number of items at the end of the file to print.  Although the type
     is signed, the value is never negative.  */
  off_t n_units = DEFAULT_N_LINES;
  int n_files;
  char **file;
  struct File_spec *F;
  int i;

  /* The number of seconds to sleep between iterations.
     During one iteration, every file name or descriptor is checked to
     see if it has changed.  */
  double sleep_interval = 1.0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  have_read_stdin = 0;

  {
    int fail;

    if (parse_obsolescent_option (argc,
				  (const char *const *) argv,
				  &n_units, &fail))
      {
	if (fail)
	  exit (EXIT_FAILURE);
	optind = 2;
      }
    else
      {
	parse_options (argc, argv, &n_units, &header_mode, &sleep_interval);
      }
  }

  /* To start printing with item N_UNITS from the start of the file, skip
     N_UNITS - 1 items.  `tail -n +0' is actually meaningless, but for Unix
     compatibility it's treated the same as `tail -n +1'.  */
  if (from_start)
    {
      if (n_units)
	--n_units;
    }

  n_files = argc - optind;
  file = argv + optind;

  if (n_files == 0)
    {
      static char *dummy_stdin = "-";
      n_files = 1;
      file = &dummy_stdin;
    }

  F = (struct File_spec *) xmalloc (n_files * sizeof (F[0]));
  for (i = 0; i < n_files; i++)
    F[i].name = file[i];

  if (header_mode == always
      || (header_mode == multiple_files && n_files > 1))
    print_headers = 1;

  for (i = 0; i < n_files; i++)
    exit_status |= tail_file (&F[i], n_units);

  if (forever)
    {
      /* This fflush appears to be required only on Solaris2.7.  */
      if (fflush (stdout) < 0)
	error (EXIT_FAILURE, errno, _("write error"));

      SETVBUF (stdout, NULL, _IONBF, 0);
      tail_forever (F, n_files, sleep_interval);
    }

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, "-");
  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
