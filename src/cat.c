/* cat -- concatenate files and print on the standard output.
   Copyright (C) 88, 90, 91, 1995-2002 Free Software Foundation, Inc.

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

/* Differences from the Unix cat:
   * Always unbuffered, -u is ignored.
   * Usually much faster than other versions of cat, the difference
   is especially apparent when using the -v option.

   By tege@sics.se, Torbjorn Granlund, advised by rms, Richard Stallman. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#ifndef _POSIX_SOURCE
# include <sys/ioctl.h>
#endif
#include "system.h"
#include "closeout.h"
#include "error.h"
#include "full-write.h"
#include "safe-read.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "cat"

#define AUTHORS N_ ("Torbjorn Granlund and Richard M. Stallman")

/* Undefine, to avoid warning about redefinition on some systems.  */
#undef max
#define max(h,i) ((h) > (i) ? (h) : (i))

/* Name under which this program was invoked.  */
char *program_name;

/* Name of input file.  May be "-".  */
static char *infile;

/* Descriptor on which input file is open.  */
static int input_desc;

/* Buffer for line numbers.
   An 11 digit counter may overflow within an hour on a P2/466,
   an 18 digit counter needs about 1000y */
#define LINE_COUNTER_BUF_LEN 20
static char line_buf[LINE_COUNTER_BUF_LEN] =
  {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '0',
    '\t', '\0'
  };

/* Position in `line_buf' where printing starts.  This will not change
   unless the number of lines is larger than 999999.  */
static char *line_num_print = line_buf + LINE_COUNTER_BUF_LEN - 8;

/* Position of the first digit in `line_buf'.  */
static char *line_num_start = line_buf + LINE_COUNTER_BUF_LEN - 3;

/* Position of the last digit in `line_buf'.  */
static char *line_num_end = line_buf + LINE_COUNTER_BUF_LEN - 3;

/* Preserves the `cat' function's local `newlines' between invocations.  */
static int newlines2 = 0;

/* Count of non-fatal error conditions.  */
static int exit_status = 0;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] [FILE]...\n\
"),
	      program_name);
      fputs (_("\
Concatenate FILE(s), or standard input, to standard output.\n\
\n\
  -A, --show-all           equivalent to -vET\n\
  -b, --number-nonblank    number nonblank output lines\n\
  -e                       equivalent to -vE\n\
  -E, --show-ends          display $ at end of each line\n\
  -n, --number             number all output lines\n\
  -s, --squeeze-blank      never more than one single blank line\n\
"), stdout);
      fputs (_("\
  -t                       equivalent to -vT\n\
  -T, --show-tabs          display TAB characters as ^I\n\
  -u                       (ignored)\n\
  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
With no FILE, or when FILE is -, read standard input.\n\
"), stdout);
#if O_BINARY
      fputs (_("\
\n\
  -B, --binary             use binary writes to the console device.\n\n\
"), stdout);
#endif
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Compute the next line number.  */

static void
next_line_num (void)
{
  char *endp = line_num_end;
  do
    {
      if ((*endp)++ < '9')
	return;
      *endp-- = '0';
    }
  while (endp >= line_num_start);
  if (line_num_start > line_buf)
    *--line_num_start = '1';
  else
    *line_buf = '>';
  if (line_num_start < line_num_print)
    line_num_print--;
}

/* Plain cat.  Copies the file behind `input_desc' to STDOUT_FILENO.  */

static void
simple_cat (
     /* Pointer to the buffer, used by reads and writes.  */
     char *buf,

     /* Number of characters preferably read or written by each read and write
        call.  */
     int bufsize)
{
  /* Actual number of characters read, and therefore written.  */
  size_t n_read;

  /* Loop until the end of the file.  */

  for (;;)
    {
      /* Read a block of input.  */

      n_read = safe_read (input_desc, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
	{
	  error (0, errno, "%s", infile);
	  exit_status = 1;
	  return;
	}

      /* End of this file?  */

      if (n_read == 0)
	break;

      /* Write this block out.  */

      {
	/* The following is ok, since we know that 0 < n_read.  */
	size_t n = n_read;
	if (full_write (STDOUT_FILENO, buf, n) != n)
	  error (EXIT_FAILURE, errno, _("write error"));
      }
    }
}

/* Cat the file behind INPUT_DESC to the file behind OUTPUT_DESC.
   Called if any option more than -u was specified.

   A newline character is always put at the end of the buffer, to make
   an explicit test for buffer end unnecessary.  */

static void
cat (
     /* Pointer to the beginning of the input buffer.  */
     char *inbuf,

     /* Number of characters read in each read call.  */
     size_t insize,

     /* Pointer to the beginning of the output buffer.  */
     char *outbuf,

     /* Number of characters written by each write call.  */
     size_t outsize,

     /* Variables that have values according to the specified options.  */
     int quote,
     int output_tabs,
     int numbers,
     int numbers_at_empty_lines,
     int mark_line_ends,
     int squeeze_empty_lines)
{
  /* Last character read from the input buffer.  */
  unsigned char ch;

  /* Pointer to the next character in the input buffer.  */
  char *bpin;

  /* Pointer to the first non-valid byte in the input buffer, i.e. the
     current end of the buffer.  */
  char *eob;

  /* Pointer to the position where the next character shall be written.  */
  char *bpout;

  /* Number of characters read by the last read call.  */
  size_t n_read;

  /* Determines how many consecutive newlines there have been in the
     input.  0 newlines makes NEWLINES -1, 1 newline makes NEWLINES 1,
     etc.  Initially 0 to indicate that we are at the beginning of a
     new line.  The "state" of the procedure is determined by
     NEWLINES.  */
  int newlines = newlines2;

#ifdef FIONREAD
  /* If nonzero, use the FIONREAD ioctl, as an optimization.
     (On Ultrix, it is not supported on NFS filesystems.)  */
  int use_fionread = 1;
#endif

  /* The inbuf pointers are initialized so that BPIN > EOB, and thereby input
     is read immediately.  */

  eob = inbuf;
  bpin = eob + 1;

  bpout = outbuf;

  for (;;)
    {
      do
	{
	  /* Write if there are at least OUTSIZE bytes in OUTBUF.  */

	  if (outbuf + outsize <= bpout)
	    {
	      char *wp = outbuf;
	      size_t remaining_bytes;
	      do
		{
		  if (full_write (STDOUT_FILENO, wp, outsize) != outsize)
		    error (EXIT_FAILURE, errno, _("write error"));
		  wp += outsize;
		  remaining_bytes = bpout - wp;
		}
	      while (outsize <= remaining_bytes);

	      /* Move the remaining bytes to the beginning of the
		 buffer.  */

	      memmove (outbuf, wp, remaining_bytes);
	      bpout = outbuf + remaining_bytes;
	    }

	  /* Is INBUF empty?  */

	  if (bpin > eob)
	    {
#ifdef FIONREAD
	      int n_to_read = 0;

	      /* Is there any input to read immediately?
		 If not, we are about to wait,
		 so write all buffered output before waiting.  */

	      if (use_fionread
		  && ioctl (input_desc, FIONREAD, &n_to_read) < 0)
		{
		  /* Ultrix returns EOPNOTSUPP on NFS;
		     HP-UX returns ENOTTY on pipes.
		     SunOS returns EINVAL and
		     More/BSD returns ENODEV on special files
		     like /dev/null.
		     Irix-5 returns ENOSYS on pipes.  */
		  if (errno == EOPNOTSUPP || errno == ENOTTY
		      || errno == EINVAL || errno == ENODEV
# ifdef ENOSYS
		      || errno == ENOSYS
# endif
		      )
		    use_fionread = 0;
		  else
		    {
		      error (0, errno, _("cannot do ioctl on `%s'"), infile);
		      exit_status = 1;
		      newlines2 = newlines;
		      return;
		    }
		}
	      if (n_to_read == 0)
#endif
		{
		  size_t n_write = bpout - outbuf;

		  if (full_write (STDOUT_FILENO, outbuf, n_write) != n_write)
		    error (EXIT_FAILURE, errno, _("write error"));
		  bpout = outbuf;
		}

	      /* Read more input into INBUF.  */

	      n_read = safe_read (input_desc, inbuf, insize);
	      if (n_read == SAFE_READ_ERROR)
		{
		  error (0, errno, "%s", infile);
		  exit_status = 1;
		  newlines2 = newlines;
		  return;
		}
	      if (n_read == 0)
		{
		  newlines2 = newlines;
		  return;
		}

	      /* Update the pointers and insert a sentinel at the buffer
		 end.  */

	      bpin = inbuf;
	      eob = bpin + n_read;
	      *eob = '\n';
	    }
	  else
	    {
	      /* It was a real (not a sentinel) newline.  */

	      /* Was the last line empty?
		 (i.e. have two or more consecutive newlines been read?)  */

	      if (++newlines > 0)
		{
		  if (newlines >= 2)
		    {
		      /* Limit this to 2 here.  Otherwise, with lots of
			 consecutive newlines, the counter could wrap
			 around at INT_MAX.  */
		      newlines = 2;

		      /* Are multiple adjacent empty lines to be substituted
			 by single ditto (-s), and this was the second empty
			 line?  */
		      if (squeeze_empty_lines)
			{
			  ch = *bpin++;
			  continue;
			}
		    }

		  /* Are line numbers to be written at empty lines (-n)?  */

		  if (numbers && numbers_at_empty_lines)
		    {
		      next_line_num ();
		      bpout = stpcpy (bpout, line_num_print);
		    }
		}

	      /* Output a currency symbol if requested (-e).  */

	      if (mark_line_ends)
		*bpout++ = '$';

	      /* Output the newline.  */

	      *bpout++ = '\n';
	    }
	  ch = *bpin++;
	}
      while (ch == '\n');

      /* Are we at the beginning of a line, and line numbers are requested?  */

      if (newlines >= 0 && numbers)
	{
	  next_line_num ();
	  bpout = stpcpy (bpout, line_num_print);
	}

      /* Here CH cannot contain a newline character.  */

      /* The loops below continue until a newline character is found,
	 which means that the buffer is empty or that a proper newline
	 has been found.  */

      /* If quoting, i.e. at least one of -v, -e, or -t specified,
	 scan for chars that need conversion.  */
      if (quote)
	{
	  for (;;)
	    {
	      if (ch >= 32)
		{
		  if (ch < 127)
		    *bpout++ = ch;
		  else if (ch == 127)
		    {
		      *bpout++ = '^';
		      *bpout++ = '?';
		    }
		  else
		    {
		      *bpout++ = 'M';
		      *bpout++ = '-';
		      if (ch >= 128 + 32)
			{
			  if (ch < 128 + 127)
			    *bpout++ = ch - 128;
			  else
			    {
			      *bpout++ = '^';
			      *bpout++ = '?';
			    }
			}
		      else
			{
			  *bpout++ = '^';
			  *bpout++ = ch - 128 + 64;
			}
		    }
		}
	      else if (ch == '\t' && output_tabs)
		*bpout++ = '\t';
	      else if (ch == '\n')
		{
		  newlines = -1;
		  break;
		}
	      else
		{
		  *bpout++ = '^';
		  *bpout++ = ch + 64;
		}

	      ch = *bpin++;
	    }
	}
      else
	{
	  /* Not quoting, neither of -v, -e, or -t specified.  */
	  for (;;)
	    {
	      if (ch == '\t' && !output_tabs)
		{
		  *bpout++ = '^';
		  *bpout++ = ch + 64;
		}
	      else if (ch != '\n')
		*bpout++ = ch;
	      else
		{
		  newlines = -1;
		  break;
		}

	      ch = *bpin++;
	    }
	}
    }
}

/* This is gross, but necessary, because of the way close_stdout
   works and because this program closes STDOUT_FILENO directly.  */
static void (*closeout_func) (void) = close_stdout;

static void
close_stdout_wrapper (void)
{
  if (closeout_func)
    (*closeout_func) ();
}

int
main (int argc, char **argv)
{
  /* Optimal size of i/o operations of output.  */
  size_t outsize;

  /* Optimal size of i/o operations of input.  */
  size_t insize;

  /* Pointer to the input buffer.  */
  char *inbuf;

  /* Pointer to the output buffer.  */
  char *outbuf;

  int c;

  /* Index in argv to processed argument.  */
  int argind;

  /* Device number of the output (file or whatever).  */
  dev_t out_dev;

  /* I-node number of the output.  */
  ino_t out_ino;

  /* Nonzero if the output file should not be the same as any input file. */
  int check_redirection = 1;

  /* Nonzero if we have ever read standard input. */
  int have_read_stdin = 0;

  struct stat stat_buf;

  /* Variables that are set according to the specified options.  */
  int numbers = 0;
  int numbers_at_empty_lines = 1;
  int squeeze_empty_lines = 0;
  int mark_line_ends = 0;
  int quote = 0;
  int output_tabs = 1;
#if O_BINARY
  int binary_files  = 0;
  int binary_output = 0;
#endif
  int file_open_mode = O_RDONLY;

/* If nonzero, call cat, otherwise call simple_cat to do the actual work. */
  int options = 0;

  static struct option const long_options[] =
  {
    {"number-nonblank", no_argument, NULL, 'b'},
    {"number", no_argument, NULL, 'n'},
    {"squeeze-blank", no_argument, NULL, 's'},
    {"show-nonprinting", no_argument, NULL, 'v'},
    {"show-ends", no_argument, NULL, 'E'},
    {"show-tabs", no_argument, NULL, 'T'},
    {"show-all", no_argument, NULL, 'A'},
#if O_BINARY
    {"binary", no_argument, NULL, 'B'},
#endif
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
  };

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Arrange to close stdout if we exit via the
     case_GETOPT_HELP_CHAR or case_GETOPT_VERSION_CHAR code.  */
  atexit (close_stdout_wrapper);

  /* Parse command line options.  */

  while ((c = getopt_long (argc, argv,
#if O_BINARY
			   "benstuvABET"
#else
			   "benstuvAET"
#endif
			   , long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'b':
	  ++options;
	  numbers = 1;
	  numbers_at_empty_lines = 0;
	  break;

	case 'e':
	  ++options;
	  mark_line_ends = 1;
	  quote = 1;
	  break;

	case 'n':
	  ++options;
	  numbers = 1;
	  break;

	case 's':
	  ++options;
	  squeeze_empty_lines = 1;
	  break;

	case 't':
	  ++options;
	  output_tabs = 0;
	  quote = 1;
	  break;

	case 'u':
	  /* We provide the -u feature unconditionally.  */
	  break;

	case 'v':
	  ++options;
	  quote = 1;
	  break;

	case 'A':
	  ++options;
	  quote = 1;
	  mark_line_ends = 1;
	  output_tabs = 0;
	  break;

#if O_BINARY
	case 'B':
	  ++options;
	  binary_files = 1;
	  break;
#endif

	case 'E':
	  ++options;
	  mark_line_ends = 1;
	  break;

	case 'T':
	  ++options;
	  output_tabs = 0;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  /* Don't close stdout on exit from here on.  */
  closeout_func = NULL;

  /* Get device, i-node number, and optimal blocksize of output.  */

  if (fstat (STDOUT_FILENO, &stat_buf) < 0)
    error (EXIT_FAILURE, errno, _("standard output"));

  outsize = ST_BLKSIZE (stat_buf);
  /* Input file can be output file for non-regular files.
     fstat on pipes returns S_IFSOCK on some systems, S_IFIFO
     on others, so the checking should not be done for those types,
     and to allow things like cat < /dev/tty > /dev/tty, checking
     is not done for device files either. */

  if (S_ISREG (stat_buf.st_mode))
    {
      out_dev = stat_buf.st_dev;
      out_ino = stat_buf.st_ino;
    }
  else
    {
      check_redirection = 0;
#ifdef lint  /* Suppress `used before initialized' warning.  */
      out_dev = 0;
      out_ino = 0;
#endif
    }

#if O_BINARY
  /* We always read and write in BINARY mode, since this is the
     best way to copy the files verbatim.  Exceptions are when
     they request line numbering, squeezing of empty lines or
     marking lines' ends: then we use text I/O, because otherwise
     -b, -s and -E would surprise users on DOS/Windows where a line
     with only CR-LF is an empty line.  (Besides, if they ask for
     one of these options, they don't care much about the original
     file contents anyway).  */
  if ((!isatty (STDOUT_FILENO)
       && !(numbers || squeeze_empty_lines || mark_line_ends))
      || binary_files)
    {
      /* Switch stdout to BINARY mode.  */
      binary_output = 1;
      SET_BINARY (STDOUT_FILENO);
      /* When stdout is in binary mode, make sure all input files are
	 also read in binary mode.  */
      file_open_mode |= O_BINARY;
    }
  else if (quote)
    {
      /* If they want to see the non-printables, let's show them
	 those CR characters as well, so make the input binary.
	 But keep console output in text mode, so that LF causes
	 both CR and LF on output, and the output is readable.  */
      file_open_mode |= O_BINARY;
      SET_BINARY (0);

      /* Setting stdin to binary switches the console device to
	 raw I/O, which also affects stdout to console.  Undo that.  */
      if (isatty (STDOUT_FILENO))
	setmode (STDOUT_FILENO, O_TEXT);
    }
#endif

  /* Check if any of the input files are the same as the output file.  */

  /* Main loop.  */

  infile = "-";
  argind = optind;

  do
    {
      if (argind < argc)
	infile = argv[argind];

      if (infile[0] == '-' && infile[1] == 0)
	{
	  have_read_stdin = 1;
	  input_desc = 0;

#if O_BINARY
	  /* Switch stdin to BINARY mode if needed.  */
	  if (binary_output)
	    {
	      int tty_in = isatty (input_desc);

	      /* If stdin is a terminal device, and it is the ONLY
		 input file (i.e. we didn't write anything to the
		 output yet), switch the output back to TEXT mode.
		 This is so "cat > xyzzy" creates a DOS-style text
		 file, like people expect.  */
	      if (tty_in && optind <= argc)
		setmode (STDOUT_FILENO, O_TEXT);
	      else
		{
		  SET_BINARY (input_desc);
# ifdef __DJGPP__
		  /* This is DJGPP-specific.  By default, switching console
		     to binary mode disables SIGINT.  But we want terminal
		     reads to be interruptible.  */
		  if (tty_in)
		    __djgpp_set_ctrl_c (1);
# endif
		}
	    }
#endif
	}
      else
	{
	  input_desc = open (infile, file_open_mode);
	  if (input_desc < 0)
	    {
	      error (0, errno, "%s", infile);
	      exit_status = 1;
	      continue;
	    }
	}

      if (fstat (input_desc, &stat_buf) < 0)
	{
	  error (0, errno, "%s", infile);
	  exit_status = 1;
	  goto contin;
	}
      insize = ST_BLKSIZE (stat_buf);

      /* Compare the device and i-node numbers of this input file with
	 the corresponding values of the (output file associated with)
	 stdout, and skip this input file if they coincide.  Input
	 files cannot be redirected to themselves.  */

      if (check_redirection
	  && stat_buf.st_dev == out_dev && stat_buf.st_ino == out_ino
	  && (input_desc != STDIN_FILENO))
	{
	  error (0, 0, _("%s: input file is output file"), infile);
	  exit_status = 1;
	  goto contin;
	}

      /* Select which version of `cat' to use. If any options (more than -u,
	 --version, or --help) were specified, use `cat', otherwise use
	 `simple_cat'.  */

      if (options == 0)
	{
	  insize = max (insize, outsize);
	  inbuf = xmalloc (insize);

	  simple_cat (inbuf, insize);
	}
      else
	{
	  inbuf = xmalloc (insize + 1);

	  /* Why are (OUTSIZE - 1 + INSIZE * 4 + LINE_COUNTER_BUF_LEN)
	     bytes allocated for the output buffer?

	     A test whether output needs to be written is done when the input
	     buffer empties or when a newline appears in the input.  After
	     output is written, at most (OUTSIZE - 1) bytes will remain in the
	     buffer.  Now INSIZE bytes of input is read.  Each input character
	     may grow by a factor of 4 (by the prepending of M-^).  If all
	     characters do, and no newlines appear in this block of input, we
	     will have at most (OUTSIZE - 1 + INSIZE * 4) bytes in the buffer.
	     If the last character in the preceding block of input was a
	     newline, a line number may be written (according to the given
	     options) as the first thing in the output buffer. (Done after the
	     new input is read, but before processing of the input begins.)
	     A line number requires seldom more than LINE_COUNTER_BUF_LEN
	     positions.  */

	  outbuf = xmalloc (outsize - 1 + insize * 4 + LINE_COUNTER_BUF_LEN);

	  cat (inbuf, insize, outbuf, outsize, quote,
	       output_tabs, numbers, numbers_at_empty_lines, mark_line_ends,
	       squeeze_empty_lines);

	  free (outbuf);
	}

      free (inbuf);

    contin:
      if (!STREQ (infile, "-") && close (input_desc) < 0)
	{
	  error (0, errno, "%s", infile);
	  exit_status = 1;
	}
    }
  while (++argind < argc);

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, _("closing standard input"));

  if (close (STDOUT_FILENO) < 0)
    error (EXIT_FAILURE, errno, _("closing standard output"));

  exit (exit_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
