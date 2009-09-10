/* split.c -- split a file into pieces.
   Copyright (C) 1988, 1991, 1995-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* By tege@sics.se, with rms.

   To do:
   * Implement -t CHAR or -t REGEX to specify break characters other
     than newline. */

#include <config.h>

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "error.h"
#include "fd-reopen.h"
#include "fcntl--.h"
#include "full-read.h"
#include "full-write.h"
#include "quote.h"
#include "safe-read.h"
#include "xfreopen.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "split"

#define AUTHORS \
  proper_name_utf8 ("Torbjorn Granlund", "Torbj\303\266rn Granlund"), \
  proper_name ("Richard M. Stallman")

#define DEFAULT_SUFFIX_LENGTH 2

/* Base name of output files.  */
static char const *outbase;

/* Name of output files.  */
static char *outfile;

/* Pointer to the end of the prefix in OUTFILE.
   Suffixes are inserted here.  */
static char *outfile_mid;

/* Length of OUTFILE's suffix.  */
static size_t suffix_length = DEFAULT_SUFFIX_LENGTH;

/* Alphabet of characters to use in suffix.  */
static char const *suffix_alphabet = "abcdefghijklmnopqrstuvwxyz";

/* Name of input file.  May be "-".  */
static char *infile;

/* Descriptor on which output file is open.  */
static int output_desc;

/* If true, print a diagnostic on standard error just before each
   output file is opened. */
static bool verbose;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  VERBOSE_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"bytes", required_argument, NULL, 'b'},
  {"lines", required_argument, NULL, 'l'},
  {"line-bytes", required_argument, NULL, 'C'},
  {"suffix-length", required_argument, NULL, 'a'},
  {"numeric-suffixes", no_argument, NULL, 'd'},
  {"verbose", no_argument, NULL, VERBOSE_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [INPUT [PREFIX]]\n\
"),
              program_name);
    fputs (_("\
Output fixed-size pieces of INPUT to PREFIXaa, PREFIXab, ...; default\n\
size is 1000 lines, and default PREFIX is `x'.  With no INPUT, or when INPUT\n\
is -, read standard input.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fprintf (stdout, _("\
  -a, --suffix-length=N   use suffixes of length N (default %d)\n\
  -b, --bytes=SIZE        put SIZE bytes per output file\n\
  -C, --line-bytes=SIZE   put at most SIZE bytes of lines per output file\n\
  -d, --numeric-suffixes  use numeric suffixes instead of alphabetic\n\
  -l, --lines=NUMBER      put NUMBER lines per output file\n\
"), DEFAULT_SUFFIX_LENGTH);
      fputs (_("\
      --verbose           print a diagnostic just before each\n\
                            output file is opened\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      emit_size_note ();
      emit_bug_reporting_address ();
    }
  exit (status);
}

/* Compute the next sequential output file name and store it into the
   string `outfile'.  */

static void
next_file_name (void)
{
  /* Index in suffix_alphabet of each character in the suffix.  */
  static size_t *sufindex;

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
      memset (outfile_mid, suffix_alphabet[0], suffix_length);
      outfile[outfile_length] = 0;
      sufindex = xcalloc (suffix_length, sizeof *sufindex);

#if ! _POSIX_NO_TRUNC && HAVE_PATHCONF && defined _PC_NAME_MAX
      /* POSIX requires that if the output file name is too long for
         its directory, `split' must fail without creating any files.
         This must be checked for explicitly on operating systems that
         silently truncate file names.  */
      {
        char *dir = dir_name (outfile);
        long name_max = pathconf (dir, _PC_NAME_MAX);
        if (0 <= name_max && name_max < base_len (last_component (outfile)))
          error (EXIT_FAILURE, ENAMETOOLONG, "%s", outfile);
        free (dir);
      }
#endif
    }
  else
    {
      /* Increment the suffix in place, if possible.  */

      size_t i = suffix_length;
      while (i-- != 0)
        {
          sufindex[i]++;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
          if (outfile_mid[i])
            return;
          sufindex[i] = 0;
          outfile_mid[i] = suffix_alphabet[sufindex[i]];
        }
      error (EXIT_FAILURE, 0, _("output file suffixes exhausted"));
    }
}

/* Write BYTES bytes at BP to an output file.
   If NEW_FILE_FLAG is true, open the next output file.
   Otherwise add to the same output file already in use.  */

static void
cwrite (bool new_file_flag, const char *bp, size_t bytes)
{
  if (new_file_flag)
    {
      if (output_desc >= 0 && close (output_desc) < 0)
        error (EXIT_FAILURE, errno, "%s", outfile);

      next_file_name ();
      if (verbose)
        fprintf (stdout, _("creating file %s\n"), quote (outfile));
      output_desc = open (outfile,
                          O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                          (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP
                           | S_IROTH | S_IWOTH));
      if (output_desc < 0)
        error (EXIT_FAILURE, errno, "%s", outfile);
    }
  if (full_write (output_desc, bp, bytes) != bytes)
    error (EXIT_FAILURE, errno, "%s", outfile);
}

/* Split into pieces of exactly N_BYTES bytes.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
bytes_split (uintmax_t n_bytes, char *buf, size_t bufsize)
{
  size_t n_read;
  bool new_file_flag = true;
  size_t to_read;
  uintmax_t to_write = n_bytes;
  char *bp_out;

  do
    {
      n_read = full_read (STDIN_FILENO, buf, bufsize);
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
                  new_file_flag = false;
                }
              break;
            }
          else
            {
              size_t w = to_write;
              cwrite (new_file_flag, bp_out, w);
              bp_out += w;
              to_read -= w;
              new_file_flag = true;
              to_write = n_bytes;
            }
        }
    }
  while (n_read == bufsize);
}

/* Split into pieces of exactly N_LINES lines.
   Use buffer BUF, whose size is BUFSIZE.  */

static void
lines_split (uintmax_t n_lines, char *buf, size_t bufsize)
{
  size_t n_read;
  char *bp, *bp_out, *eob;
  bool new_file_flag = true;
  uintmax_t n = 0;

  do
    {
      n_read = full_read (STDIN_FILENO, buf, bufsize);
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
  while (n_read == bufsize);
}

/* Split into pieces that are as large as possible while still not more
   than N_BYTES bytes, and are split on line boundaries except
   where lines longer than N_BYTES bytes occur.
   FIXME: Allow N_BYTES to be any uintmax_t value, and don't require a
   buffer of size N_BYTES, in case N_BYTES is very large.  */

static void
line_bytes_split (size_t n_bytes)
{
  size_t n_read;
  char *bp;
  bool eof = false;
  size_t n_buffered = 0;
  char *buf = xmalloc (n_bytes);

  do
    {
      /* Fill up the full buffer size from the input file.  */

      n_read = full_read (STDIN_FILENO, buf + n_buffered, n_bytes - n_buffered);
      if (n_read == SAFE_READ_ERROR)
        error (EXIT_FAILURE, errno, "%s", infile);

      n_buffered += n_read;
      if (n_buffered != n_bytes)
        {
          if (n_buffered == 0)
            break;
          eof = true;
        }

      /* Find where to end this chunk.  */
      bp = buf + n_buffered;
      if (n_buffered == n_bytes)
        {
          while (bp > buf && bp[-1] != '\n')
            bp--;
        }

      /* If chunk has no newlines, use all the chunk.  */
      if (bp == buf)
        bp = buf + n_buffered;

      /* Output the chars as one output file.  */
      cwrite (true, buf, bp - buf);

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
  enum
    {
      type_undef, type_bytes, type_byteslines, type_lines, type_digits
    } split_type = type_undef;
  size_t in_blk_size;		/* optimal block size of input file device */
  char *buf;			/* file i/o buffer */
  size_t page_size = getpagesize ();
  uintmax_t n_units;
  static char const multipliers[] = "bEGKkMmPTYZ0";
  int c;
  int digits_optind = 0;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Parse command line options.  */

  infile = bad_cast ( "-");
  outbase = bad_cast ("x");

  while (1)
    {
      /* This is the argv-index of the option we will read next.  */
      int this_optind = optind ? optind : 1;

      c = getopt_long (argc, argv, "0123456789C:a:b:dl:", longopts, NULL);
      if (c == -1)
        break;

      switch (c)
        {
        case 'a':
          {
            unsigned long tmp;
            if (xstrtoul (optarg, NULL, 10, &tmp, "") != LONGINT_OK
                || SIZE_MAX / sizeof (size_t) < tmp)
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
          if (xstrtoumax (optarg, NULL, 10, &n_units, multipliers) != LONGINT_OK
              || n_units == 0)
            {
              error (0, 0, _("%s: invalid number of bytes"), optarg);
              usage (EXIT_FAILURE);
            }
          break;

        case 'l':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_lines;
          if (xstrtoumax (optarg, NULL, 10, &n_units, "") != LONGINT_OK
              || n_units == 0)
            {
              error (0, 0, _("%s: invalid number of lines"), optarg);
              usage (EXIT_FAILURE);
            }
          break;

        case 'C':
          if (split_type != type_undef)
            FAIL_ONLY_ONE_WAY ();
          split_type = type_byteslines;
          if (xstrtoumax (optarg, NULL, 10, &n_units, multipliers) != LONGINT_OK
              || n_units == 0 || SIZE_MAX < n_units)
            {
              error (0, 0, _("%s: invalid number of bytes"), optarg);
              usage (EXIT_FAILURE);
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
          if (!DECIMAL_DIGIT_ACCUMULATE (n_units, c - '0', uintmax_t))
            {
              char buffer[INT_BUFSIZE_BOUND (uintmax_t)];
              error (EXIT_FAILURE, 0,
                     _("line count option -%s%c... is too large"),
                     umaxtostr (n_units, buffer), c);
            }
          break;

        case 'd':
          suffix_alphabet = "0123456789";
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

  /* Handle default case.  */
  if (split_type == type_undef)
    {
      split_type = type_lines;
      n_units = 1000;
    }

  if (n_units == 0)
    {
      error (0, 0, _("invalid number of lines: 0"));
      usage (EXIT_FAILURE);
    }

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

  /* Open the input file.  */
  if (! STREQ (infile, "-")
      && fd_reopen (STDIN_FILENO, infile, O_RDONLY, 0) < 0)
    error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
           quote (infile));

  /* Binary I/O is safer when bytecounts are used.  */
  if (O_BINARY && ! isatty (STDIN_FILENO))
    xfreopen (NULL, "rb", stdin);

  /* No output file is open now.  */
  output_desc = -1;

  /* Get the optimal block size of input device and make a buffer.  */

  if (fstat (STDIN_FILENO, &stat_buf) != 0)
    error (EXIT_FAILURE, errno, "%s", infile);
  in_blk_size = io_blksize (stat_buf);

  buf = ptr_align (xmalloc (in_blk_size + 1 + page_size - 1), page_size);

  switch (split_type)
    {
    case type_digits:
    case type_lines:
      lines_split (n_units, buf, in_blk_size);
      break;

    case type_bytes:
      bytes_split (n_units, buf, in_blk_size);
      break;

    case type_byteslines:
      line_bytes_split (n_units);
      break;

    default:
      abort ();
    }

  if (close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, "%s", infile);
  if (output_desc >= 0 && close (output_desc) < 0)
    error (EXIT_FAILURE, errno, "%s", outfile);

  exit (EXIT_SUCCESS);
}
