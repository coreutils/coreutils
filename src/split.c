/* split.c -- split a file into pieces.
   Copyright (C) 1988, 1991 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* By tege@sics.se, with rms.

   To do:
   * Implement -t CHAR or -t REGEX to specify break characters other
     than newline. */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "version.h"

char *xmalloc ();
void error ();

static int convint ();
static int isdigits ();
static int stdread ();
static void line_bytes_split ();
static void bytes_split ();
static void cwrite ();
static void lines_split ();
static void next_file_name ();

/* The name this program was run with. */
char *program_name;

/* Base name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Pointer to the end of OUTFILE. */
static char *outfile_end;

/* Status for outfile name generation.  */
static unsigned outfile_count = -1;
static unsigned outfile_name_limit = 25 * 26;
static unsigned outfile_name_generation = 1;

/* Name of input file.  May be "-".  */
static char *infile;

/* Descriptor on which input file is open.  */
static int input_desc;

/* Descriptor on which output file is open.  */
static int output_desc;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output then exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"lines", required_argument, NULL, 'l'},
  {"line-bytes", required_argument, NULL, 'C'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
usage (status, reason)
     int status;
     char *reason;
{
  if (reason != NULL)
    fprintf (stderr, "%s: %s\n", program_name, reason);

  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s [OPTION] [INPUT [PREFIX]]\n\
",
	      program_name);
    printf ("\
\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of lines per output file\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -l, --lines=NUMBER      put NUMBER lines per output file\n\
  -NUMBER                 same as -l NUMBER\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
\n\
SIZE may have a multiplier suffix: b for 512, k for 1K, m for 1 Meg.\n\
With no PREFIX, use x.  With no INPUT, or when INPUT is -, read\n\
standard input.\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char *argv[];
{
  struct stat stat_buf;
  int num;			/* numeric argument from command line */
  enum
    {
      type_undef, type_bytes, type_byteslines, type_lines, type_digits
    } split_type = type_undef;
  int in_blk_size;		/* optimal block size of input file device */
  char *buf;			/* file i/o buffer */
  int accum = 0;
  char *outbase;
  int c;
  int digits_optind = 0;

  program_name = argv[0];

  /* Parse command line options.  */

  infile = "-";
  outbase = "x";

  while (1)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;

      c = getopt_long (argc, argv, "0123456789b:l:C:", longopts, (int *) 0);
      if (c == EOF)
	break;

      switch (c)
	{
	case 0:
	  break;

	case 'b':
	  if (split_type != type_undef)
	    usage (2, "cannot split in more than one way");
	  split_type = type_bytes;
	  if (convint (optarg, &accum) == -1)
	    usage (2, "invalid number of bytes");
	  break;

	case 'l':
	  if (split_type != type_undef)
	    usage (2, "cannot split in more than one way");
	  split_type = type_lines;
	  if (!isdigits (optarg))
	    usage (2, "invalid number of lines");
	  accum = atoi (optarg);
	  break;

	case 'C':
	  if (split_type != type_undef)
	    usage (2, "cannot split in more than one way");
	  split_type = type_byteslines;
	  if (convint (optarg, &accum) == -1)
	    usage (2, "invalid number of bytes");
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
	    usage (2, "cannot split in more than one way");
	  if (digits_optind != 0 && digits_optind != this_optind)
	    accum = 0;		/* More than one number given; ignore other. */
	  digits_optind = this_optind;
	  split_type = type_digits;
	  accum = accum * 10 + c - '0';
	  break;

	default:
	  usage (2, (char *)0);
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0, (char *)0);

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      accum = 1000;
    }

  if (accum < 1)
    usage (2, "invalid number");
  num = accum;

  /* Get out the filename arguments.  */

  if (optind < argc)
    infile = argv[optind++];

  if (optind < argc)
    outbase = argv[optind++];

  if (optind < argc)
    usage (2, "too many arguments");

  /* Open the input file.  */
  if (!strcmp (infile, "-"))
    input_desc = 0;
  else
    {
      input_desc = open (infile, O_RDONLY);
      if (input_desc < 0)
	error (1, errno, "%s", infile);
    }

  /* No output file is open now.  */
  output_desc = -1;

  /* Copy the output file prefix so we can add suffixes to it.
     26**29 is certainly enough output files!  */

  outfile = xmalloc (strlen (outbase) + 30);
  strcpy (outfile, outbase);
  outfile_mid = outfile + strlen (outfile);
  outfile_end = outfile_mid + 2;
  bzero (outfile_mid, 30);
  outfile_mid[0] = 'a';
  outfile_mid[1] = 'a' - 1;  /* first call to next_file_name makes it an 'a' */

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (input_desc, &stat_buf) < 0)
    error (1, errno, "%s", infile);
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
    error (1, errno, "%s", infile);
  if (output_desc >= 0 && close (output_desc) < 0)
    error (1, errno, "%s", outfile);

  exit (0);
}

/* Return nonzero if the string STR is composed entirely of decimal digits.  */

static int
isdigits (str)
     char *str;
{
  do
    {
      if (!ISDIGIT (*str))
	return 0;
      str++;
    }
  while (*str);
  return 1;
}

/* Put the value of the number in STR into *VAL.
   STR can specify a positive integer, optionally ending in `k'
   to mean kilo or `m' to mean mega.
   Return 0 if STR is valid, -1 if not. */

static int
convint (str, val)
     char *str;
     int *val;
{
  int multiplier = 1;
  int arglen = strlen (str);

  if (arglen > 1)
    {
      switch (str[arglen - 1])
	{
	case 'b':
	  multiplier = 512;
	  str[arglen - 1] = '\0';
	  break;
	case 'k':
	  multiplier = 1024;
	  str[arglen - 1] = '\0';
	  break;
	case 'm':
	  multiplier = 1048576;
	  str[arglen - 1] = '\0';
	  break;
	}
    }
  if (!isdigits (str))
    return -1;
  *val = atoi (str) * multiplier;
  return 0;
}

/* Split into pieces of exactly NCHARS bytes.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
bytes_split (nchars, buf, bufsize)
     int nchars;
     char *buf;
     int bufsize;
{
  int n_read;
  int new_file_flag = 1;
  int to_read;
  int to_write = nchars;
  char *bp_out;

  do
    {
      n_read = stdread (buf, bufsize);
      if (n_read < 0)
        error (1, errno, "%s", infile);
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
	  else
	    {
	      cwrite (new_file_flag, bp_out, to_write);
	      bp_out += to_write;
	      to_read -= to_write;
	      new_file_flag = 1;
	      to_write = nchars;
	    }
	}
    }
  while (n_read == bufsize);
}

/* Split into pieces of exactly NLINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (nlines, buf, bufsize)
     int nlines;
     char *buf;
     int bufsize;
{
  int n_read;
  char *bp, *bp_out, *eob;
  int new_file_flag = 1;
  int n = 0;

  do
    {
      n_read = stdread (buf, bufsize);
      if (n_read < 0)
	error (1, errno, "%s", infile);
      bp = bp_out = buf;
      eob = bp + n_read;
      *eob = '\n';
      for (;;)
	{
	  while (*bp++ != '\n')
	    ;			/* this semicolon takes most of the time */
	  if (bp > eob)
	    {
	      if (eob != bp_out) /* do not write 0 bytes! */
		{
		  cwrite (new_file_flag, bp_out, eob - bp_out);
		  new_file_flag = 0;
		}
	      break;
	    }
	  else
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
line_bytes_split (nchars)
     int nchars;
{
  int n_read;
  char *bp;
  int eof = 0;
  int n_buffered = 0;
  char *buf = (char *) xmalloc (nchars);

  do
    {
      /* Fill up the full buffer size from the input file.  */

      n_read = stdread (buf + n_buffered, nchars - n_buffered);
      if (n_read < 0)
	error (1, errno, "%s", infile);

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
	 down to be the start of the next chunk.  */
      n_buffered -= bp - buf;
      if (n_buffered > 0)
	bcopy (bp, buf, n_buffered);
    }
  while (!eof);
  free (buf);
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is nonzero, open the next output file.
   Otherwise add to the same output file already in use.  */

static void
cwrite (new_file_flag, bp, bytes)
     int new_file_flag;
     char *bp;
     int bytes;
{
  if (new_file_flag)
    {
      if (output_desc >= 0 && close (output_desc) < 0)
	error (1, errno, "%s", outfile);

      next_file_name ();
      output_desc = open (outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (output_desc < 0)
	error (1, errno, "%s", outfile);
    }
  if (write (output_desc, bp, bytes) < 0)
    error (1, errno, "%s", outfile);
}

/* Read NCHARS bytes from the input file into BUF.
   Return the number of bytes successfully read.
   If this is less than NCHARS, do not call `stdread' again.  */

static int
stdread (buf, nchars)
     char *buf;
     int nchars;
{
  int n_read;
  int to_be_read = nchars;

  while (to_be_read)
    {
      n_read = read (input_desc, buf, to_be_read);
      if (n_read < 0)
	return -1;
      if (n_read == 0)
	break;
      to_be_read -= n_read;
      buf += n_read;
    }
  return nchars - to_be_read;
}

/* Compute the next sequential output file name suffix and store it
   into the string `outfile' at the position pointed to by `outfile_mid'.  */

static void
next_file_name ()
{
  int x;
  char *ne;

  outfile_count++;
  if (outfile_count < outfile_name_limit)
    {
      for (ne = outfile_end - 1; ; ne--)
	{
	  x = *ne;
	  if (x != 'z')
	    break;
	  *ne = 'a';
	}
      *ne = x + 1;
      return;
    }

  outfile_count = 0;
  outfile_name_limit *= 26;
  outfile_name_generation++;
  *outfile_mid++ = 'z';
  for (x = 0; x <= outfile_name_generation; x++)
    outfile_mid[x] = 'a';
  outfile_end += 2;
}
