/* split.c -- split a file into pieces.
   Copyright (C) 88, 91, 1995-2003 Free Software Foundation, Inc.

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

/* By tege@sics.se, with rms.

   To do:
   * Implement -t CHAR or -t REGEX to specify break characters other
     than newline. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "dirname.h"
#include "error.h"
#include "full-read.h"
#include "full-write.h"
#include "posixver.h"
#include "safe-read.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "split"

#define AUTHORS N_ ("Torbjorn Granlund and Richard M. Stallman")

#define DEFAULT_SUFFIX_LENGTH 2

/* The name this program was run with. */
char *program_name;

/* Base name of output files.  */
static char const *outbase;

/* Name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Length of OUTFILE's suffix.  */
static size_t suffix_length = DEFAULT_SUFFIX_LENGTH;

/* Name of input file.  May be "-".  */
static char *infile;

/* Descriptor on which input file is open.  */
static int input_desc;

/* Descriptor on which output file is open.  */
static int output_desc;

/* If nonzero, print a diagnostic on standard error just before each
   output file is opened. */
static int verbose;

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"lines", required_argument, NULL, 'l'},
  {"line-bytes", required_argument, NULL, 'C'},
  {"suffix-length", required_argument, NULL, 'a'},
  {"verbose", no_argument, &verbose, 0},
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
Usage: %s [OPTION] [INPUT [PREFIX]]\n\
"),
	      program_name);
    fputs (_("\
Output fixed-size pieces of INPUT to PREFIXaa, PREFIXab, ...; default\n\
PREFIX is `x'.  With no INPUT, or when INPUT is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fprintf (stdout, _("\
  -a, --suffix-length=N   use suffixes of length N (default %d)\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of lines per output file\n\
  -l, --lines=NUMBER      put NUMBER lines per output file\n\
"), DEFAULT_SUFFIX_LENGTH);
      fputs (_("\
      --verbose           print a diagnostic to standard error just\n\
                            before each output file is opened\n\
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

/* Compute the next sequential output file name and store it into the
   string `outfile'.  */

static void
next_file_name (void)
{
  if (! outfile)
    {
      /* Allocate and initialize the first file name.  */

      size_t outbase_length = strlen (outbase);
      size_t outfile_length = outbase_length + suffix_length;
      if (outfile_length + 1 < outbase_length)
	xalloc_die ();
      outfile = xmalloc (outfile_length + 1);
      outfile_mid = outfile + outbase_length;
      memcpy (outfile, outbase, outbase_length);
      memset (outfile_mid, 'a', suffix_length);
      outfile[outfile_length] = 0;

#if ! _POSIX_NO_TRUNC && HAVE_PATHCONF && defined _PC_NAME_MAX
      /* POSIX requires that if the output file name is too long for
	 its directory, `split' must fail without creating any files.
	 This must be checked for explicitly on operating systems that
	 silently truncate file names.  */
      {
	char *dir = dir_name (outfile);
	long name_max = pathconf (dir, _PC_NAME_MAX);
	if (0 <= name_max && name_max < base_len (base_name (outfile)))
	  error (EXIT_FAILURE, ENAMETOOLONG, "%s", outfile);
	free (dir);
      }
#endif
    }
  else
    {
      /* Increment the suffix in place, if possible.  */

      char *p;
      for (p = outfile_mid + suffix_length; outfile_mid < p; *--p = 'a')
	if (p[-1]++ != 'z')
	  return;
      error (EXIT_FAILURE, 0, _("Output file suffixes exhausted"));
    }
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is nonzero, open the next output file.
   Otherwise add to the same output file already in use.  */

static void
cwrite (int new_file_flag, const char *bp, size_t bytes)
{
  if (new_file_flag)
    {
      if (output_desc >= 0 && close (output_desc) < 0)
	error (EXIT_FAILURE, errno, "%s", outfile);

      next_file_name ();
      if (verbose)
	fprintf (stderr, _("creating file `%s'\n"), outfile);
      output_desc = open (outfile,
			  O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
      if (output_desc < 0)
	error (EXIT_FAILURE, errno, "%s", outfile);
    }
  if (full_write (output_desc, bp, bytes) != bytes)
    error (EXIT_FAILURE, errno, "%s", outfile);
}

/* Split into pieces of exactly NCHARS bytes.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
bytes_split (size_t nchars, char *buf, size_t bufsize)
{
  size_t n_read;
  int new_file_flag = 1;
  size_t to_read;
  size_t to_write = nchars;
  char *bp_out;

  do
    {
      n_read = full_read (input_desc, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
        error (EXIT_FAILURE, errno, "%s", infile);
      bp_out = buf;
      to_read = n_read;
      for (;;)
	{
	  if (to_read < to_write)
	    {
	      if (to_read)	/* do not write 0 bytes! */
		{
		  cwrite (new_file_flag, bp_out, to_read);
		  to_write -= to_read;
		  new_file_flag = 0;
		}
	      break;
	    }

	  cwrite (new_file_flag, bp_out, to_write);
	  bp_out += to_write;
	  to_read -= to_write;
	  new_file_flag = 1;
	  to_write = nchars;
	}
    }
  while (n_read == bufsize);
}

/* Split into pieces of exactly NLINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (size_t nlines, char *buf, size_t bufsize)
{
  size_t n_read;
  char *bp, *bp_out, *eob;
  int new_file_flag = 1;
  size_t n = 0;

  do
    {
      n_read = full_read (input_desc, buf, bufsize);
      if (n_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, "%s", infile);
      bp = bp_out = buf;
      eob = bp + n_read;
      *eob = '\n';
      for (;;)
	{
	  bp = memchr (bp, '\n', eob - bp + 1);
	  if (bp == eob)
	    {
	      if (eob != bp_out) /* do not write 0 bytes! */
		{
		  size_t len = eob - bp_out;
		  cwrite (new_file_flag, bp_out, len);
		  new_file_flag = 0;
		}
	      break;
	    }

	  ++bp;
	  if (++n >= nlines)
	    {
	      cwrite (new_file_flag, bp_out, bp - bp_out);
	      bp_out = bp;
	      new_file_flag = 1;
	      n = 0;
	    }
	}
    }
  while (n_read == bufsize);
}

/* Split into pieces that are as large as possible while still not more
   than NCHARS bytes, and are split on line boundaries except
   where lines longer than NCHARS bytes occur. */

static void
line_bytes_split (size_t nchars)
{
  size_t n_read;
  char *bp;
  int eof = 0;
  size_t n_buffered = 0;
  char *buf = (char *) xmalloc (nchars);

  do
    {
      /* Fill up the full buffer size from the input file.  */

      n_read = full_read (input_desc, buf + n_buffered, nchars - n_buffered);
      if (n_read == SAFE_READ_ERROR)
	error (EXIT_FAILURE, errno, "%s", infile);

      n_buffered += n_read;
      if (n_buffered != nchars)
	eof = 1;

      /* Find where to end this chunk.  */
      bp = buf + n_buffered;
      if (n_buffered == nchars)
	{
	  while (bp > buf && bp[-1] != '\n')
	    bp--;
	}

      /* If chunk has no newlines, use all the chunk.  */
      if (bp == buf)
	bp = buf + n_buffered;

      /* Output the chars as one output file.  */
      cwrite (1, buf, bp - buf);

      /* Discard the chars we just output; move rest of chunk
	 down to be the start of the next chunk.  Source and
	 destination probably overlap.  */
      n_buffered -= bp - buf;
      if (n_buffered > 0)
	memmove (buf, bp, n_buffered);
    }
  while (!eof);
  free (buf);
}

#define FAIL_ONLY_ONE_WAY()					\
  do								\
    {								\
      error (0, 0, _("cannot split in more than one way"));	\
      usage (EXIT_FAILURE);					\
    }								\
  while (0)

int
main (int argc, char **argv)
{
  struct stat stat_buf;
  size_t num;			/* numeric argument from command line */
  enum
    {
      type_undef, type_bytes, type_byteslines, type_lines, type_digits
    } split_type = type_undef;
  size_t in_blk_size;		/* optimal block size of input file device */
  char *buf;			/* file i/o buffer */
  size_t accum = 0;
  int c;
  int digits_optind = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Parse command line options.  */

  infile = "-";
  outbase = "x";

  while (1)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;
      long int tmp_long;

      c = getopt_long (argc, argv, "0123456789C:a:b:l:", longopts, NULL);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;

	case 'a':
	  {
	    unsigned long tmp;
	    if (xstrtoul (optarg, NULL, 10, &tmp, "") != LONGINT_OK
		|| SIZE_MAX < tmp)
	      {
		error (0, 0, _("%s: invalid suffix length"), optarg);
		usage (EXIT_FAILURE);
	      }
	    suffix_length = tmp;
	  }
	  break;

	case 'b':
	  if (split_type != type_undef)
	    FAIL_ONLY_ONE_WAY ();
	  split_type = type_bytes;
	  if (xstrtol (optarg, NULL, 10, &tmp_long, "bkm") != LONGINT_OK
	      || tmp_long < 0 || tmp_long > INT_MAX)
	    {
	      error (0, 0, _("%s: invalid number of bytes"), optarg);
	      usage (EXIT_FAILURE);
	    }
	  accum = /* FIXME: */ (int) tmp_long;
	  break;

	case 'l':
	  if (split_type != type_undef)
	    FAIL_ONLY_ONE_WAY ();
	  split_type = type_lines;
	  if (xstrtol (optarg, NULL, 10, &tmp_long, "") != LONGINT_OK
	      || tmp_long < 0 || tmp_long > INT_MAX)
	    {
	      error (0, 0, _("%s: invalid number of lines"), optarg);
	      usage (EXIT_FAILURE);
	    }
	  accum = /* FIXME */ (int) tmp_long;
	  break;

	case 'C':
	  if (split_type != type_undef)
	    FAIL_ONLY_ONE_WAY ();
	  split_type = type_byteslines;
	  if (xstrtol (optarg, NULL, 10, &tmp_long, "bkm") != LONGINT_OK
	      || tmp_long < 0 ||  tmp_long > INT_MAX)
	    {
	      error (0, 0, _("%s: invalid number of bytes"), optarg);
	      usage (EXIT_FAILURE);
	    }
	  accum = /* FIXME */ (int) tmp_long;
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
	  if (split_type != type_undef && split_type != type_digits)
	    FAIL_ONLY_ONE_WAY ();
	  if (digits_optind != 0 && digits_optind != this_optind)
	    accum = 0;		/* More than one number given; ignore other. */
	  digits_optind = this_optind;
	  split_type = type_digits;
	  accum = accum * 10 + c - '0';
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (digits_optind && 200112 <= posix2_version ())
    {
      error (0, 0, _("`-%d' option is obsolete; use `-l %d'"), accum, accum);
      usage (EXIT_FAILURE);
    }

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      accum = 1000;
    }

  if (accum < 1)
    {
      error (0, 0, _("invalid number"));
      usage (EXIT_FAILURE);
    }
  num = accum;

  /* Get out the filename arguments.  */

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outbase = argv[optind++];

  if (optind < argc)
    {
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  /* Open the input file.  */
  if (STREQ (infile, "-"))
    input_desc = 0;
  else
    {
      input_desc = open (infile, O_RDONLY);
      if (input_desc < 0)
	error (EXIT_FAILURE, errno, "%s", infile);
    }
  /* Binary I/O is safer when bytecounts are used.  */
  SET_BINARY (input_desc);

  /* No output file is open now.  */
  output_desc = -1;

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (input_desc, &stat_buf) < 0)
    error (EXIT_FAILURE, errno, "%s", infile);
  in_blk_size = ST_BLKSIZE (stat_buf);

  buf = xmalloc (in_blk_size + 1);

  switch (split_type)
    {
    case type_digits:
    case type_lines:
      lines_split (num, buf, in_blk_size);
      break;

    case type_bytes:
      bytes_split (num, buf, in_blk_size);
      break;

    case type_byteslines:
      line_bytes_split (num);
      break;

    default:
      abort ();
    }

  if (close (input_desc) < 0)
    error (EXIT_FAILURE, errno, "%s", infile);
  if (output_desc >= 0 && close (output_desc) < 0)
    error (EXIT_FAILURE, errno, "%s", outfile);

  exit (EXIT_SUCCESS);
}
