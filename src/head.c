/* head -- output first part of file(s)
   Copyright (C) 89, 90, 91, 1995-2003 Free Software Foundation, Inc.

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

/* Options: (see usage)
   Reads from standard input if no files are given or when a filename of
   ``-'' is encountered.
   By default, filename headers are printed only if more than one file
   is given.
   By default, prints the first 10 lines (head -n 10).

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"

#include "closeout.h"
#include "error.h"
#include "full-write.h"
#include "full-read.h"
#include "inttostr.h"
#include "posixver.h"
#include "quote.h"
#include "safe-read.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "head"

#define AUTHORS "David MacKenzie"

/* Number of lines/chars/blocks to head. */
#define DEFAULT_NUMBER 10

/* Size of atomic reads. */
#define BUFSIZE (512 * 8)

/* Useful only when eliding tail bytes or lines.
   If nonzero, skip the is-regular-file test used to determine whether
   to use the lseek optimization.  Instead, use the more general (and
   more expensive) code unconditionally. Intended solely for testing.  */
static int presume_input_pipe;

/* If nonzero, print filename headers. */
static int print_headers;

/* When to print the filename banners. */
enum header_mode
{
  multiple_files, always, never
};

/* Options corresponding to header_mode values.  */
static char const header_mode_option[][4] = { "", " -v", " -q" };

/* The name this program was run with. */
char *program_name;

/* Have we ever read standard input?  */
static int have_read_stdin;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  PRESUME_INPUT_PIPE_OPTION = CHAR_MAX + 1
};

static struct option const long_options[] =
{
  {"bytes", required_argument, NULL, 'c'},
  {"lines", required_argument, NULL, 'n'},
  {"presume-input-pipe", no_argument, NULL,
   PRESUME_INPUT_PIPE_OPTION}, /* do not document */
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 'q'},
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
      fputs (_("\
Print first 10 lines of each FILE to standard output.\n\
With more than one FILE, precede each with a header giving the file name.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -c, --bytes=SIZE         print first SIZE bytes\n\
  -n, --lines=NUMBER       print first NUMBER lines instead of first 10\n\
"), stdout);
      fputs (_("\
  -q, --quiet, --silent    never print headers giving file names\n\
  -v, --verbose            always print headers giving file names\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
SIZE may have a multiplier suffix: b for 512, k for 1K, m for 1 Meg.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
write_header (const char *filename)
{
  static int first_file = 1;

  printf ("%s==> %s <==\n", (first_file ? "" : "\n"), filename);
  first_file = 0;
}

enum Copy_fd_status
  {
    COPY_FD_OK = 0,
    COPY_FD_READ_ERROR,
    COPY_FD_WRITE_ERROR,
    COPY_FD_UNEXPECTED_EOF
  };

/* Copy no more than N_BYTES from file descriptor SRC_FD to O_STREAM.
   Return an appropriate indication of success or failure. */

static enum Copy_fd_status
copy_fd (int src_fd, FILE *o_stream, uintmax_t n_bytes)
{
  char buf[BUFSIZE];
  const size_t buf_size = sizeof (buf);

  /* Copy the file contents.  */
  while (0 < n_bytes)
    {
      size_t n_to_read = MIN (buf_size, n_bytes);
      size_t n_read = safe_read (src_fd, buf, n_to_read);
      if (n_read == SAFE_READ_ERROR)
	return COPY_FD_READ_ERROR;

      /* assert (n_read <= n_bytes); */
      n_bytes -= n_read;

      if (n_read == 0 && n_bytes != 0)
	return COPY_FD_UNEXPECTED_EOF;

      if (fwrite (buf, 1, n_read, o_stream) < n_read)
	return COPY_FD_WRITE_ERROR;
    }

  return COPY_FD_OK;
}

static int
elide_tail_bytes_pipe (const char *filename, int fd, uintmax_t n_elide_0)
{
  size_t n_elide = n_elide_0;

#ifndef HEAD_TAIL_PIPE_READ_BUFSIZE
# define HEAD_TAIL_PIPE_READ_BUFSIZE BUFSIZE
#endif
#define READ_BUFSIZE HEAD_TAIL_PIPE_READ_BUFSIZE

  /* If we're eliding no more than this many bytes, then it's ok to allocate
     more memory in order to use a more time-efficient algorithm.
     FIXME: use a fraction of available memory instead, as in sort.  */
#ifndef HEAD_TAIL_PIPE_BYTECOUNT_THRESHOLD
# define HEAD_TAIL_PIPE_BYTECOUNT_THRESHOLD 1024 * 1024
#endif

#if HEAD_TAIL_PIPE_BYTECOUNT_THRESHOLD < 2 * READ_BUFSIZE
  "HEAD_TAIL_PIPE_BYTECOUNT_THRESHOLD must be at least 2 * READ_BUFSIZE"
#endif

  if (SIZE_MAX < n_elide_0 + READ_BUFSIZE)
    {
      char umax_buf[INT_BUFSIZE_BOUND (uintmax_t)];
      error (EXIT_FAILURE, 0, _("%s: number of bytes is large"),
	     umaxtostr (n_elide_0, umax_buf));
    }

  /* Two cases to consider...
     1) n_elide is small enough that we can afford to double-buffer:
        allocate 2 * (READ_BUFSIZE + n_elide) bytes
     2) n_elide is too big for that, so we allocate only
        (READ_BUFSIZE + n_elide) bytes

     CAUTION: do not fail (out of memory) when asked to elide
     a ridiculous amount, but when given only a small input.  */

  if (n_elide <= HEAD_TAIL_PIPE_BYTECOUNT_THRESHOLD)
    {
      int fail = 0;
      bool first = true;
      bool eof = false;
      size_t n_to_read = READ_BUFSIZE + n_elide;
      unsigned int i;
      char *b[2];
      b[0] = xmalloc (2 * n_to_read);
      b[1] = b[0] + n_to_read;

      for (i = 0; ! eof ; i = !i)
	{
	  size_t n_read = full_read (fd, b[i], n_to_read);
	  size_t delta = 0;
	  if (n_read < n_to_read)
	    {
	      if (errno != 0)
		{
		  error (0, errno, _("error reading %s"), quote (filename));
		  fail = 1;
		  break;
		}

	      /* reached EOF */
	      if (n_read <= n_elide)
		{
		  if (first)
		    {
		      /* The input is no larger than the number of bytes
			 to elide.  So there's nothing to output, and
			 we're done.  */
		    }
		  else
		    {
		      delta = n_elide - n_read;
		    }
		}
	      eof = true;
	    }

	  /* Output any (but maybe just part of the) elided data from
	     the previous round.  */
	  if ( ! first)
	    {
	      /* Don't bother checking for errors here.
		 If there's a failure, the test of the following
		 fwrite or in close_stdout will catch it.  */
	      fwrite (b[!i] + READ_BUFSIZE, 1, n_elide - delta, stdout);
	    }
	  first = false;

	  if (n_elide < n_read
	      && fwrite (b[i], 1, n_read - n_elide, stdout) < n_read - n_elide)
	    {
	      error (0, errno, _("write error"));
	      fail = 1;
	      break;
	    }
	}

      free (b[0]);
      return fail;
    }
  else
    {
      /* Read blocks of size READ_BUFSIZE, until we've read at least n_elide
	 bytes.  Then, for each new buffer we read, also write an old one.  */

      int fail = 0;
      bool eof = false;
      size_t n_read;
      bool buffered_enough;
      size_t i, i_next;
      char **b;
      /* Round n_elide up to a multiple of READ_BUFSIZE.  */
      size_t rem = READ_BUFSIZE - (n_elide % READ_BUFSIZE);
      size_t n_elide_round = n_elide + rem;
      size_t n_bufs = n_elide_round / READ_BUFSIZE + 1;
      b = xcalloc (n_bufs, sizeof *b);

      buffered_enough = false;
      for (i = 0, i_next = 1; !eof; i = i_next, i_next = (i_next + 1) % n_bufs)
	{
	  if (b[i] == NULL)
	    b[i] = xmalloc (READ_BUFSIZE);
	  n_read = full_read (fd, b[i], READ_BUFSIZE);
	  if (n_read < READ_BUFSIZE)
	    {
	      if (errno != 0)
		{
		  error (0, errno, _("error reading %s"), quote (filename));
		  fail = 1;
		  goto free_mem;
		}
	      eof = true;
	    }

	  if (i + 1 == n_bufs)
	    buffered_enough = true;

	  if (buffered_enough)
	    {
	      if (fwrite (b[i_next], 1, n_read, stdout) < n_read)
		{
		  error (0, errno, _("write error"));
		  fail = 1;
		  goto free_mem;
		}
	    }
	}

      /* Output any remainder: rem bytes from b[i] + n_read.  */
      if (rem)
	{
	  if (buffered_enough)
	    {
	      size_t n_bytes_left_in_b_i = READ_BUFSIZE - n_read;
	      if (rem < n_bytes_left_in_b_i)
		{
		  fwrite (b[i] + n_read, 1, rem, stdout);
		}
	      else
		{
		  fwrite (b[i] + n_read, 1, n_bytes_left_in_b_i, stdout);
		  fwrite (b[i_next], 1, rem - n_bytes_left_in_b_i, stdout);
		}
	    }
	  else if (i + 1 == n_bufs)
	    {
	      /* This happens when n_elide < file_size < n_elide_round.

		 |READ_BUF.|
		 |                      |  rem |
		 |---------!---------!---------!---------|
		 |---- n_elide ---------|
		 |                      | x |
		 |                   |y |
		 |---- file size -----------|
		 |                   |n_read|
		 |---- n_elide_round ----------|
	       */
	      size_t y = READ_BUFSIZE - rem;
	      size_t x = n_read - y;
	      fwrite (b[i_next], 1, x, stdout);
	    }
	}

    free_mem:;
      for (i = 0; i < n_bufs; i++)
	if (b[i])
	  free (b[i]);
      free (b);

      return fail;
    }
}

/* FIXME: describe.  */

/* NOTE Getting the length by seeking is not robust, due to a race condition.
   The problem arises if the file grows or shrinks between the length
   determination and the actual reading.  */

static int
elide_tail_bytes_file (const char *filename, int fd, uintmax_t n_elide)
{
  struct stat stats;

  /* We need binary input, since `tail' relies on `lseek' and byte counts,
     while binary output will preserve the style (Unix/DOS) of text file.  */
  SET_BINARY2 (fd, STDOUT_FILENO);

  if (presume_input_pipe || fstat (fd, &stats) || ! S_ISREG (stats.st_mode))
    {
      return elide_tail_bytes_pipe (filename, fd, n_elide);
    }
  else
    {
      off_t current_pos, end_pos;
      uintmax_t bytes_remaining;
      off_t diff;
      enum Copy_fd_status err;

      if ((current_pos = lseek (fd, (off_t) 0, SEEK_CUR)) == -1
	  || (end_pos = lseek (fd, (off_t) 0, SEEK_END)) == -1)
	{
	  error (0, errno, _("cannot lseek %s"), quote (filename));
	  return 1;
	}

      /* Be careful here.  The current position may actually be
	 beyond the end of the file.  */
      bytes_remaining = (diff = end_pos - current_pos) < 0 ? 0 : diff;

      if (bytes_remaining <= n_elide)
	return 0;

      /* Seek back to `current' position, then copy the required
	 number of bytes from fd.  */
      if (lseek (fd, (off_t) 0, current_pos) == -1)
	{
	  error (0, errno, _("%s: cannot lseek back to original position"),
		 quote (filename));
	  return 1;
	}

      err = copy_fd (fd, stdout, bytes_remaining - n_elide);
      if (err == COPY_FD_OK)
	return 0;

      switch (err)
	{
	case COPY_FD_READ_ERROR:
	  error (0, errno, "error reading %s", quote (filename));
	  break;
	case COPY_FD_WRITE_ERROR:
	  error (0, errno, "error writing %s", quote (filename));
	  break;
	case COPY_FD_UNEXPECTED_EOF:
	  error (0, errno, "%s: file has shrunk too much", quote (filename));
	  break;
	default:
	  abort ();
	}
      return 1;
    }
}

/* FIXME: comment */

static int
elide_tail_lines_pipe (const char *filename, int fd, uintmax_t n_elide)
{
  /* FIXME: working here */
  abort ();
}

/* FIXME: comment */

static int
elide_tail_lines_file (const char *filename, int fd, uintmax_t n_elide)
{
  struct stat stats;

  /* We need binary input, since `tail' relies on `lseek' and byte counts,
     while binary output will preserve the style (Unix/DOS) of text file.  */
  SET_BINARY2 (fd, STDOUT_FILENO);

  if (presume_input_pipe || fstat (fd, &stats) || ! S_ISREG (stats.st_mode))
    {
      return elide_tail_lines_pipe (filename, fd, n_elide);
    }
  else
    {
      /* Find the offset, OFF, of the Nth newline from the end,
	 but not counting the last byte of the file.
	 If found, write from current position to OFF, inclusive.
	 Otherwise, just return 0.  */

      /* FIXME: working here */
      abort ();
    }
}

static int
head_bytes (const char *filename, int fd, uintmax_t bytes_to_write)
{
  char buffer[BUFSIZE];
  size_t bytes_to_read = BUFSIZE;

  /* Need BINARY I/O for the byte counts to be accurate.  */
  SET_BINARY2 (fd, fileno (stdout));

  while (bytes_to_write)
    {
      size_t bytes_read;
      if (bytes_to_write < bytes_to_read)
	bytes_to_read = bytes_to_write;
      bytes_read = safe_read (fd, buffer, bytes_to_read);
      if (bytes_read == SAFE_READ_ERROR)
	{
	  error (0, errno, _("error reading %s"), quote (filename));
	  return 1;
	}
      if (bytes_read == 0)
	break;
      if (fwrite (buffer, 1, bytes_read, stdout) < bytes_read)
	error (EXIT_FAILURE, errno, _("write error"));
      bytes_to_write -= bytes_read;
    }
  return 0;
}

static int
head_lines (const char *filename, int fd, uintmax_t lines_to_write)
{
  char buffer[BUFSIZE];

  /* Need BINARY I/O for the byte counts to be accurate.  */
  /* FIXME: do we really need this when counting *lines*?  */
  SET_BINARY2 (fd, fileno (stdout));

  while (lines_to_write)
    {
      size_t bytes_read = safe_read (fd, buffer, BUFSIZE);
      size_t bytes_to_write = 0;

      if (bytes_read == SAFE_READ_ERROR)
	{
	  error (0, errno, _("error reading %s"), quote (filename));
	  return 1;
	}
      if (bytes_read == 0)
	break;
      while (bytes_to_write < bytes_read)
	if (buffer[bytes_to_write++] == '\n' && --lines_to_write == 0)
	  {
	    off_t n_bytes_past_EOL = bytes_read - bytes_to_write;
	    /* If we have read more data than that on the specified number
	       of lines, try to seek back to the position we would have
	       gotten to had we been reading one byte at a time.  */
	    if (lseek (fd, -n_bytes_past_EOL, SEEK_CUR) < 0)
	      {
		int e = errno;
		struct stat st;
		if (fstat (fd, &st) != 0 || S_ISREG (st.st_mode))
		  error (0, e, _("cannot reposition file pointer for %s"),
			 quote (filename));
	      }
	    break;
	  }
      if (fwrite (buffer, 1, bytes_to_write, stdout) < bytes_to_write)
	error (EXIT_FAILURE, errno, _("write error"));
    }
  return 0;
}

static int
head (const char *filename, int fd, uintmax_t n_units, int count_lines,
      int elide_from_end)
{
  if (print_headers)
    write_header (filename);

  if (elide_from_end)
    {
      if (count_lines)
	{
	  return elide_tail_lines_file (filename, fd, n_units);
	}
      else
	{
	  return elide_tail_bytes_file (filename, fd, n_units);
	}
    }
  if (count_lines)
    return head_lines (filename, fd, n_units);
  else
    return head_bytes (filename, fd, n_units);
}

static int
head_file (const char *filename, uintmax_t n_units, int count_lines,
	   int elide_from_end)
{
  int fd;
  int fail;

  if (STREQ (filename, "-"))
    {
      have_read_stdin = 1;
      fd = STDIN_FILENO;
      filename = _("standard input");
    }
  else
    {
      fd = open (filename, O_RDONLY);
      if (fd < 0)
	{
	  error (0, errno, "cannot open %s for reading", quote (filename));
	  return 1;
	}
    }

  fail = head (filename, fd, n_units, count_lines, elide_from_end);
  if (fd != STDIN_FILENO && close (fd) == -1)
    {
      error (0, errno, "closing %s", quote (filename));
      fail = 1;
    }
  return fail;
}

/* Convert a string of decimal digits, N_STRING, with a single, optional suffix
   character (b, k, or m) to an integral value.  Upon successful conversion,
   return that value.  If it cannot be converted, give a diagnostic and exit.
   COUNT_LINES indicates whether N_STRING is a number of bytes or a number
   of lines.  It is used solely to give a more specific diagnostic.  */

static uintmax_t
string_to_integer (int count_lines, const char *n_string)
{
  strtol_error s_err;
  uintmax_t n;

  s_err = xstrtoumax (n_string, NULL, 10, &n, "bkm");

  if (s_err == LONGINT_OVERFLOW)
    {
      error (EXIT_FAILURE, 0,
	     _("%s: %s is so large that it is not representable"), n_string,
	     count_lines ? _("number of lines") : _("number of bytes"));
    }

  if (s_err != LONGINT_OK)
    {
      error (EXIT_FAILURE, 0, "%s: %s", n_string,
	     (count_lines
	      ? _("invalid number of lines")
	      : _("invalid number of bytes")));
    }

  return n;
}

int
main (int argc, char **argv)
{
  enum header_mode header_mode = multiple_files;
  int exit_status = 0;
  int c;
  size_t i;

  /* Number of items to print. */
  uintmax_t n_units = DEFAULT_NUMBER;

  /* If nonzero, interpret the numeric argument as the number of lines.
     Otherwise, interpret it as the number of bytes.  */
  int count_lines = 1;

  /* Elide the specified number of lines or bytes, counting from
     the end of the file.  */
  int elide_from_end = 0;

  /* Initializer for file_list if no file-arguments
     were specified on the command line.  */
  static char const *const default_file_list[] = {"-", NULL};
  char const *const *file_list;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  have_read_stdin = 0;

  print_headers = 0;

  if (1 < argc && argv[1][0] == '-' && ISDIGIT (argv[1][1]))
    {
      char *a = argv[1];
      char *n_string = ++a;
      char *end_n_string;
      char multiplier_char = 0;

      /* Old option syntax; a dash, one or more digits, and one or
	 more option letters.  Move past the number. */
      do ++a;
      while (ISDIGIT (*a));

      /* Pointer to the byte after the last digit.  */
      end_n_string = a;

      /* Parse any appended option letters. */
      for (; *a; a++)
	{
	  switch (*a)
	    {
	    case 'c':
	      count_lines = 0;
	      multiplier_char = 0;
	      break;

	    case 'b':
	    case 'k':
	    case 'm':
	      count_lines = 0;
	      multiplier_char = *a;
	      break;

	    case 'l':
	      count_lines = 1;
	      break;

	    case 'q':
	      header_mode = never;
	      break;

	    case 'v':
	      header_mode = always;
	      break;

	    default:
	      error (0, 0, _("unrecognized option `-%c'"), *a);
	      usage (EXIT_FAILURE);
	    }
	}

      if (200112 <= posix2_version ())
	{
	  error (0, 0, _("`-%s' option is obsolete; use `-%c %.*s%.*s%s'"),
		 n_string, count_lines ? 'n' : 'c',
		 (int) (end_n_string - n_string), n_string,
		 multiplier_char != 0, &multiplier_char,
		 header_mode_option[header_mode]);
	  usage (EXIT_FAILURE);
	}

      /* Append the multiplier character (if any) onto the end of
	 the digit string.  Then add NUL byte if necessary.  */
      *end_n_string = multiplier_char;
      if (multiplier_char)
	*(++end_n_string) = 0;

      n_units = string_to_integer (count_lines, n_string);

      /* Make the options we just parsed invisible to getopt. */
      argv[1] = argv[0];
      argv++;
      argc--;

      /* FIXME: allow POSIX options if there were obsolescent ones?  */

    }

  while ((c = getopt_long (argc, argv, "c:n:qv", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case PRESUME_INPUT_PIPE_OPTION:
	  presume_input_pipe = 1;
	  break;

	case 'c':
	  count_lines = 0;
	  elide_from_end = (*optarg == '-');
	  if (elide_from_end)
	    ++optarg;
	  n_units = string_to_integer (count_lines, optarg);
	  break;

	case 'n':
	  count_lines = 1;
	  elide_from_end = (*optarg == '-');
	  if (elide_from_end)
	    ++optarg;
	  n_units = string_to_integer (count_lines, optarg);
	  break;

	case 'q':
	  header_mode = never;
	  break;

	case 'v':
	  header_mode = always;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (header_mode == always
      || (header_mode == multiple_files && optind < argc - 1))
    print_headers = 1;

  if ( ! count_lines && elide_from_end && OFF_T_MAX < n_units)
    {
      char buf[INT_BUFSIZE_BOUND (uintmax_t)];
      error (EXIT_FAILURE, 0, _("%s: number of bytes is too large"),
	     umaxtostr (n_units, buf));
    }

  file_list = (argc - optind == 0
	       ? default_file_list
	       : (char const *const *) &argv[optind]);

  for (i = 0; file_list[i]; ++i)
    exit_status |= head_file (file_list[i], n_units, count_lines, elide_from_end);

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, "-");

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
