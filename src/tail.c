/* tail -- output the last part of file(s)
   Copyright (C) 89, 90, 91, 95, 96, 1997, 1998 Free Software Foundation, Inc.

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

   Options:
   -b			Tail by N 512-byte blocks.
   -c, --bytes=N[bkm]	Tail by N bytes
			[or 512-byte blocks, kilobytes, or megabytes].
   -f, --follow		Loop forever trying to read more characters at the
			end of the file, on the assumption that the file
			is growing.  Ignored if reading from a pipe.
   -n, --lines=N	Tail by N lines.
   -q, --quiet, --silent	Never print filename headers.
   -v, --verbose		Always print filename headers.

   If a number (N) starts with a `+', begin printing with the Nth item
   from the start of each file, instead of from the end.

   Reads from standard input if no files are given or when a filename of
   ``-'' is encountered.
   By default, filename headers are printed only more than one file
   is given.
   By default, prints the last 10 lines (tail -n 10).

   Original version by Paul Rubin <phr@ocf.berkeley.edu>.
   Extensions by David MacKenzie <djm@gnu.ai.mit.edu>.
   tail -f for multiple files by Ian Lance Taylor <ian@airs.com>.  */

#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "xstrtoul.h"
#include "error.h"

#ifndef OFF_T_MIN
# define OFF_T_MIN TYPE_MINIMUM (off_t)
#endif

#ifndef OFF_T_MAX
# define OFF_T_MAX TYPE_MAXIMUM (off_t)
#endif

/* Disable assertions.  Some systems have broken assert macros.  */
#define NDEBUG 1

#define XWRITE(fd, buffer, n_bytes)					\
  do									\
    {									\
      assert ((fd) == 1);						\
      assert ((n_bytes) >= 0);						\
      if (n_bytes > 0 && fwrite ((buffer), 1, (n_bytes), stdout) == 0)	\
	error (EXIT_FAILURE, errno, _("write error"));			\
    }									\
  while (0)

/* Number of items to tail.  */
#define DEFAULT_N_LINES 10

/* Size of atomic reads.  */
#ifndef BUFSIZ
# define BUFSIZ (512 * 8)
#endif

/* If nonzero, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static int count_lines;

/* If nonzero, read from the end of one file until killed.  */
static int forever;

/* If nonzero, read from the end of multiple files until killed.  */
static int forever_multiple;

/* Array of file descriptors if forever_multiple is 1.  */
static int *file_descs;

/* Array of file sizes if forever_multiple is 1.  */
static off_t *file_sizes;

/* If nonzero, count from start of file instead of end.  */
static int from_start;

/* If nonzero, print filename headers.  */
static int print_headers;

/* When to print the filename banners.  */
enum header_mode
{
  multiple_files, always, never
};

int safe_read ();

/* The name this program was run with.  */
char *program_name;

/* Nonzero if we have ever read standard input.  */
static int have_read_stdin;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output then exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"bytes", required_argument, NULL, 'c'},
  {"follow", no_argument, NULL, 'f'},
  {"lines", required_argument, NULL, 'n'},
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 'q'},
  {"verbose", no_argument, NULL, 'v'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
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
Print last 10 lines of each FILE to standard output.\n\
With more than one FILE, precede each with a header giving the file name.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
  -c, --bytes=N            output the last N bytes\n\
  -f, --follow             output appended data as the file grows\n\
  -n, --lines=N            output the last N lines, instead of last 10\n\
  -q, --quiet, --silent    never output headers giving file names\n\
  -v, --verbose            always output headers giving file names\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
\n\
If the first character of N (the number of bytes or lines) is a `+',\n\
print beginning with the Nth item from the start of each file, otherwise,\n\
print the last N items in the file.  N may have a multiplier suffix:\n\
b for 512, k for 1024, m for 1048576 (1 Meg).  A first OPTION of -VALUE\n\
or +VALUE is treated like -n VALUE or -n +VALUE unless VALUE has one of\n\
the [bkm] suffix multipliers, in which case it is treated like -c VALUE\n\
or -c +VALUE.\n\
"));
      puts (_("\nReport bugs to <textutils-bugs@gnu.org>."));
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
write_header (const char *filename, const char *comment)
{
  static int first_file = 1;

  printf ("%s==> %s%s%s <==\n", (first_file ? "" : "\n"), filename,
	  (comment ? ": " : ""),
	  (comment ? comment : ""));
  first_file = 0;
}

/* Print the last N_LINES lines from the end of file FD.
   Go backward through the file, reading `BUFSIZ' bytes at a time (except
   probably the first), until we hit the start of the file or have
   read NUMBER newlines.
   POS starts out as the length of the file (the offset of the last
   byte of the file + 1).
   Return 0 if successful, 1 if an error occurred.  */

static int
file_lines (const char *filename, int fd, long int n_lines, off_t pos)
{
  char buffer[BUFSIZ];
  int bytes_read;
  int i;			/* Index into `buffer' for scanning.  */

  if (n_lines == 0)
    return 0;

  /* Set `bytes_read' to the size of the last, probably partial, buffer;
     0 < `bytes_read' <= `BUFSIZ'.  */
  bytes_read = pos % BUFSIZ;
  if (bytes_read == 0)
    bytes_read = BUFSIZ;
  /* Make `pos' a multiple of `BUFSIZ' (0 if the file is short), so that all
     reads will be on block boundaries, which might increase efficiency.  */
  pos -= bytes_read;
  lseek (fd, pos, SEEK_SET);
  bytes_read = safe_read (fd, buffer, bytes_read);
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  /* Count the incomplete line on files that don't end with a newline.  */
  if (bytes_read && buffer[bytes_read - 1] != '\n')
    --n_lines;

  do
    {
      /* Scan backward, counting the newlines in this bufferfull.  */
      for (i = bytes_read - 1; i >= 0; i--)
	{
	  /* Have we counted the requested number of newlines yet?  */
	  if (buffer[i] == '\n' && n_lines-- == 0)
	    {
	      /* If this newline wasn't the last character in the buffer,
	         print the text after it.  */
	      if (i != bytes_read - 1)
		XWRITE (STDOUT_FILENO, &buffer[i + 1], bytes_read - (i + 1));
	      return 0;
	    }
	}
      /* Not enough newlines in that bufferfull.  */
      if (pos == 0)
	{
	  /* Not enough lines in the file; print the entire file.  */
	  lseek (fd, (off_t) 0, SEEK_SET);
	  return 0;
	}
      pos -= BUFSIZ;
      lseek (fd, pos, SEEK_SET);
    }
  while ((bytes_read = safe_read (fd, buffer, BUFSIZ)) > 0);
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  return 0;
}

/* Print the last N_LINES lines from the end of the standard input,
   open for reading as pipe FD.
   Buffer the text as a linked list of LBUFFERs, adding them as needed.
   Return 0 if successful, 1 if an error occured.  */

static int
pipe_lines (const char *filename, int fd, long int n_lines)
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

  first = last = (LBUFFER *) xmalloc (sizeof (LBUFFER));
  first->nbytes = first->nlines = 0;
  first->next = NULL;
  tmp = (LBUFFER *) xmalloc (sizeof (LBUFFER));

  /* Input is always read into a fresh buffer.  */
  while ((tmp->nbytes = safe_read (fd, tmp->buffer, BUFSIZ)) > 0)
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
  if (tmp->nbytes == -1)
    {
      error (0, errno, "%s", filename);
      errors = 1;
      free ((char *) tmp);
      goto free_lbuffers;
    }

  free ((char *) tmp);

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
      char *cp;

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
  XWRITE (STDOUT_FILENO, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    XWRITE (STDOUT_FILENO, tmp->buffer, tmp->nbytes);

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
pipe_bytes (const char *filename, int fd, off_t n_bytes)
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
      error (0, errno, "%s", filename);
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
  XWRITE (STDOUT_FILENO, &tmp->buffer[i], tmp->nbytes - i);

  for (tmp = tmp->next; tmp; tmp = tmp->next)
    XWRITE (STDOUT_FILENO, tmp->buffer, tmp->nbytes);

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
   Return 1 on error, 0 if ok.  */

static int
start_bytes (const char *filename, int fd, off_t n_bytes)
{
  char buffer[BUFSIZ];
  int bytes_read = 0;

  while (n_bytes > 0 && (bytes_read = safe_read (fd, buffer, BUFSIZ)) > 0)
    n_bytes -= bytes_read;
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  else if (n_bytes < 0)
    XWRITE (STDOUT_FILENO, &buffer[bytes_read + n_bytes], -n_bytes);
  return 0;
}

/* Skip N_LINES lines at the start of file or pipe FD, and print
   any extra characters that were read beyond that.
   Return 1 on error, 0 if ok.  */

static int
start_lines (const char *filename, int fd, long int n_lines)
{
  char buffer[BUFSIZ];
  int bytes_read = 0;
  int bytes_to_skip = 0;

  while (n_lines && (bytes_read = safe_read (fd, buffer, BUFSIZ)) > 0)
    {
      bytes_to_skip = 0;
      while (bytes_to_skip < bytes_read)
	if (buffer[bytes_to_skip++] == '\n' && --n_lines == 0)
	  break;
    }
  if (bytes_read == -1)
    {
      error (0, errno, "%s", filename);
      return 1;
    }
  else if (bytes_to_skip < bytes_read)
    {
      XWRITE (STDOUT_FILENO, &buffer[bytes_to_skip],
	      bytes_read - bytes_to_skip);
    }
  return 0;
}

/* Display file FILENAME from the current position in FD to the end.
   If `forever' is nonzero, keep reading from the end of the file
   until killed.  Return the number of bytes read from the file.  */

static long
dump_remainder (const char *filename, int fd)
{
  char buffer[BUFSIZ];
  int bytes_read;
  long total;

  total = 0;
output:
  while ((bytes_read = safe_read (fd, buffer, BUFSIZ)) > 0)
    {
      XWRITE (STDOUT_FILENO, buffer, bytes_read);
      total += bytes_read;
    }
  if (bytes_read == -1)
    error (EXIT_FAILURE, errno, "%s", filename);
  if (forever)
    {
      fflush (stdout);
      sleep (1);
      goto output;
    }
  else
    {
      if (forever_multiple)
	fflush (stdout);
    }

  return total;
}

/* Tail NFILES (>1) files forever until killed.  The file names are in
   NAMES.  The open file descriptors are in `file_descs', and the size
   at which we stopped tailing them is in `file_sizes'.  We loop over
   each of them, doing an fstat to see if they have changed size.  If
   none of them have changed size in one iteration, we sleep for a
   second and try again.  We do this until the user interrupts us.  */

static void
tail_forever (char **names, int nfiles)
{
  int last;

  last = -1;

  while (1)
    {
      int i;
      int changed;

      changed = 0;
      for (i = 0; i < nfiles; i++)
	{
	  struct stat stats;

	  if (file_descs[i] < 0)
	    continue;
	  if (fstat (file_descs[i], &stats) < 0)
	    {
	      error (0, errno, "%s", names[i]);
	      file_descs[i] = -1;
	      continue;
	    }
	  if (stats.st_size == file_sizes[i])
	    continue;

	  /* This file has changed size.  Print out what we can, and
	     then keep looping.  */

	  changed = 1;

	  if (stats.st_size < file_sizes[i])
	    {
	      write_header (names[i], _("file truncated"));
	      last = i;
	      lseek (file_descs[i], stats.st_size, SEEK_SET);
	      file_sizes[i] = stats.st_size;
	      continue;
	    }

	  if (i != last)
	    {
	      if (print_headers)
		write_header (names[i], NULL);
	      last = i;
	    }
	  file_sizes[i] += dump_remainder (names[i], file_descs[i]);
	}

      /* If none of the files changed size, sleep.  */
      if (! changed)
	sleep (1);
    }
}

/* Output the last N_BYTES bytes of file FILENAME open for reading in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_bytes (const char *filename, int fd, off_t n_bytes)
{
  struct stat stats;

  /* FIXME: resolve this like in dd.c.  */
  /* Use fstat instead of checking for errno == ESPIPE because
     lseek doesn't work on some special files but doesn't return an
     error, either.  */
  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  if (from_start)
    {
      if (S_ISREG (stats.st_mode))
	lseek (fd, n_bytes, SEEK_CUR);
      else if (start_bytes (filename, fd, n_bytes))
	return 1;
      dump_remainder (filename, fd);
    }
  else
    {
      if (S_ISREG (stats.st_mode))
	{
	  off_t current_pos, end_pos;
	  size_t bytes_remaining;

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
	      error (0, errno, "%s", filename);
	      return 1;
	    }

	  if (bytes_remaining <= n_bytes)
	    {
	      /* From the current position to end of file, there are no
		 more bytes than have been requested.  So reposition the
		 file pointer to the incoming current position and print
		 everything after that.  */
	      lseek (fd, current_pos, SEEK_SET);
	    }
	  else
	    {
	      /* There are more bytes remaining than were requested.
		 Back up.  */
	      lseek (fd, -n_bytes, SEEK_END);
	    }
	  dump_remainder (filename, fd);
	}
      else
	return pipe_bytes (filename, fd, n_bytes);
    }
  return 0;
}

/* Output the last N_LINES lines of file FILENAME open for reading in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_lines (const char *filename, int fd, long int n_lines)
{
  struct stat stats;
  off_t length;

  if (fstat (fd, &stats))
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  if (from_start)
    {
      if (start_lines (filename, fd, n_lines))
	return 1;
      dump_remainder (filename, fd);
    }
  else
    {
      /* Use file_lines only if FD refers to a regular file with
         its file pointer positioned at beginning of file.  */
      /* FIXME: adding the lseek conjunct is a kludge.
	 Once there's a reasonable test suite, fix the true culprit:
	 file_lines.  file_lines shouldn't presume that the input
	 file pointer is initially positioned to beginning of file.  */
      if (S_ISREG (stats.st_mode)
	  && lseek (fd, (off_t) 0, SEEK_CUR) == (off_t) 0)
	{
	  length = lseek (fd, (off_t) 0, SEEK_END);
	  if (length != 0 && file_lines (filename, fd, n_lines, length))
	    return 1;
	  dump_remainder (filename, fd);
	}
      else
	return pipe_lines (filename, fd, n_lines);
    }
  return 0;
}

/* Display the last N_UNITS units of file FILENAME, open for reading
   in FD.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail (const char *filename, int fd, off_t n_units)
{
  if (count_lines)
    return tail_lines (filename, fd, (long) n_units);
  else
    return tail_bytes (filename, fd, n_units);
}

/* Display the last N_UNITS units of file FILENAME.
   "-" for FILENAME means the standard input.
   FILENUM is this file's index in the list of files the user gave.
   Return 0 if successful, 1 if an error occurred.  */

static int
tail_file (const char *filename, off_t n_units, int filenum)
{
  int fd, errors;
  struct stat stats;

  if (!strcmp (filename, "-"))
    {
      have_read_stdin = 1;
      filename = _("standard input");
      if (print_headers)
	write_header (filename, NULL);
      errors = tail (filename, 0, n_units);
      if (forever_multiple)
	{
	  if (fstat (0, &stats) < 0)
	    {
	      error (0, errno, _("standard input"));
	      errors = 1;
	    }
	  else if (!S_ISREG (stats.st_mode))
	    {
	      error (0, 0,
		   _("standard input: cannot follow end of non-regular file"));
	      errors = 1;
	    }
	  if (errors)
	    file_descs[filenum] = -1;
	  else
	    {
	      file_descs[filenum] = 0;
	      file_sizes[filenum] = stats.st_size;
	    }
	}
    }
  else
    {
      /* Not standard input.  */
      fd = open (filename, O_RDONLY);
      if (fd == -1)
	{
	  if (forever_multiple)
	    file_descs[filenum] = -1;
	  error (0, errno, "%s", filename);
	  errors = 1;
	}
      else
	{
	  if (print_headers)
	    write_header (filename, NULL);
	  errors = tail (filename, fd, n_units);
	  if (forever_multiple)
	    {
	      if (fstat (fd, &stats) < 0)
		{
		  error (0, errno, "%s", filename);
		  errors = 1;
		}
	      else if (!S_ISREG (stats.st_mode))
		{
		  error (0, 0, _("%s: cannot follow end of non-regular file"),
			 filename);
		  errors = 1;
		}
	      if (errors)
		{
		  close (fd);
		  file_descs[filenum] = -1;
		}
	      else
		{
		  file_descs[filenum] = fd;
		  file_sizes[filenum] = stats.st_size;
		}
	    }
	  else
	    {
	      if (close (fd))
		{
		  error (0, errno, "%s", filename);
		  errors = 1;
		}
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

  int t_from_start;
  int t_count_lines;
  int t_forever;

  /* With the obsolescent form, there is one option string and
     (technically) at most one file argument.  But we allow two or more
     by default.  */
  if (argc < 2)
    return 0;

  /* If I were implementing this in Perl, the rest of this function
     would be essentially this single statement:
     return $p ne '-' && $p ne '-c' && $p =~ /^[+-]\d*[cl]?f?$/;  */

  /* Test this:
     if (STREQ (p, "-") || STREQ (p, "-c"))
     but without using strcmp.  */
  if (p[0] == '-' && (p[1] == 0 || (p[1] == 'c' && p[2] == 0)))
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
  if (*p == 'c')
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
      unsigned long int tmp_ulong;
      char *end;
      s_err = xstrtoul (n_string, &end, 0, &tmp_ulong, NULL);
      if (s_err == LONGINT_OK && tmp_ulong <= OFF_T_MAX)
	*n_units = (off_t) tmp_ulong;
      else
	{
	  /* Extract a NUL-terminated string for the error message.  */
	  size_t len = n_string_end - n_string;
	  char *n_string_tmp = xmalloc (len + 1);

	  strncpy (n_string_tmp, n_string, len);
	  n_string_tmp[len] = '\0';

	  error (0, 0,
		 _("%s: %s is so large that it is not representable"),
		 n_string_tmp, (count_lines
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
	  int posix_pedantic = (getenv ("POSIXLY_CORRECT") != NULL);

	  /* When POSIXLY_CORRECT is set, enforce the `at most one
	     file argument' requirement.  */
	  if (posix_pedantic)
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

      /* Set globals.  */
      from_start = t_from_start;
      count_lines = t_count_lines;
      forever = t_forever;
    }

  return 1;
}

static void
parse_options (int argc, char **argv,
	       off_t *n_units, enum header_mode *header_mode)
{
  int c;

  count_lines = 1;
  forever = forever_multiple = from_start = print_headers = 0;

  while ((c = getopt_long (argc, argv, "c:n:fqv", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
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
	    unsigned long int tmp_ulong;
	    s_err = xstrtoul (optarg, NULL, 0, &tmp_ulong, "bkm");
	    if (s_err == LONGINT_INVALID)
	      {
		error (EXIT_FAILURE, 0, "%s: %s", optarg,
		       (c == 'n'
			? _("invalid number of lines")
			: _("invalid number of bytes")));
	      }
	    if (s_err != LONGINT_OK || tmp_ulong > OFF_T_MAX)
	      {
		error (EXIT_FAILURE, 0,
		       _("%s: %s is so large that it is not representable"),
		       optarg,
		       c == 'n' ? _("number of lines") : _("number of bytes"));
	      }
	    *n_units = (off_t) tmp_ulong;
	  }
	  break;

	case 'f':
	  forever = 1;
	  break;

	case 'q':
	  *header_mode = never;
	  break;

	case 'v':
	  *header_mode = always;
	  break;

	default:
	  usage (1);
	}
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

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  have_read_stdin = 0;

  {
    int found_obsolescent;
    int fail;
    found_obsolescent = parse_obsolescent_option (argc,
						  (const char *const *) argv,
						  &n_units, &fail);
    if (found_obsolescent)
      {
	if (fail)
	  exit (EXIT_FAILURE);
	optind = 2;
      }
    else
      {
	parse_options (argc, argv, &n_units, &header_mode);
      }
  }

  if (show_version)
    {
      printf ("tail (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (EXIT_SUCCESS);
    }

  if (show_help)
    usage (0);

  /* To start printing with item N_UNITS from the start of the file, skip
     N_UNITS - 1 items.  `tail +0' is actually meaningless, but for Unix
     compatibility it's treated the same as `tail +1'.  */
  if (from_start)
    {
      if (n_units)
	--n_units;
    }

  n_files = argc - optind;
  file = argv + optind;

  if (n_files > 1 && forever)
    {
      forever_multiple = 1;
      forever = 0;
      file_descs = (int *) xmalloc (n_files * sizeof (int));
      file_sizes = (off_t *) xmalloc (n_files * sizeof (off_t));
    }

  if (header_mode == always
      || (header_mode == multiple_files && n_files > 1))
    print_headers = 1;

  if (n_files == 0)
    {
      exit_status |= tail_file ("-", n_units, 0);
    }
  else
    {
      int i;
      for (i = 0; i < n_files; i++)
	exit_status |= tail_file (file[i], n_units, i);

      if (forever_multiple)
	tail_forever (file, n_files);
    }

  if (have_read_stdin && close (0) < 0)
    error (EXIT_FAILURE, errno, "-");
  if (fclose (stdout) == EOF)
    error (EXIT_FAILURE, errno, _("write error"));
  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
