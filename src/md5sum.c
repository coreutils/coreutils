/* Compute MD5 checksum of files or strings according to the definition
   of MD5 in RFC 1321 from April 1992.
   Copyright (C) 1995 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "md5.h"
#include "getline.h"
#include "system.h"
#include "error.h"
#include "version.h"

/* Most systems do not distinguish between external and internal
   text representations.  */
#if UNIX || __UNIX__ || unix || __unix__ || _POSIX_VERSION
# define OPENOPTS(BINARY) "r"
#else
# ifdef MSDOS
#  define TEXT1TO1 "rb"
#  define TEXTCNVT "r"
# else
#  if defined VMS
#   define TEXT1TO1 "rb", "ctx=stm"
#   define TEXTCNVT "r", "ctx=stm"
#  else
    /* The following line is intended to evoke an error.
       Using #error is not portable enough.  */
    "Cannot determine system type."
#  endif
# endif
# define OPENOPTS(BINARY) ((BINARY) != 0 ? TEXT1TO1 : TEXTCNVT)
#endif

#if _LIBC || STDC_HEADERS
# define TOLOWER(c) tolower (c)
#else
# define TOLOWER(c) (ISUPPER (c) ? tolower (c) : (c))
#endif

/* Nonzero if any of the files read were the standard input. */
static int have_read_stdin;

/* FIXME: comment. */
static int quiet = 0;

/* FIXME: comment. */
static int verbose = 0;

/* The name this program was run with.  */
char *program_name;

static const struct option long_options[] =
{
  { "binary", no_argument, 0, 'b' },
  { "check", no_argument, 0, 'c' },
  { "help", no_argument, 0, 'h' },
  { "quiet", no_argument, 0, 'q' },
  { "string", required_argument, 0, 's' },
  { "text", no_argument, 0, 't' },
  { "verbose", no_argument, 0, 'v' },
  { "version", no_argument, 0, 'V' },
  { NULL, 0, NULL, 0 }
};

char *xmalloc ();

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    printf (_("\
Usage: %s [OPTION] [FILE]...\n\
  or:  %s [OPTION] --string=STRING\n\
  or:  %s [OPTION] --check [FILE]\n\
Print or check MD5 checksums.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
  -h, --help              display this help and exit\n\
  -q, --quiet             don't show anything, status code shows success\n\
  -v, --verbose           verbose output level\n\
  -V, --version           output version information and exit\n\
\n\
  -b, --binary            read files in binary mode (default)\n\
  -t, --text              read files in text mode\n\
\n\
  -c, --check             check MD5 sums against given list\n\
  -s, --string=STRING     compute checksum for STRING\n\
\n\
The sums are computed as described in RFC 1321.  When checking, the input\n\
should be a former output of this program.  The default mode is to print\n\
a line with checksum, type, and name for each FILE.\n"),
	    program_name, program_name, program_name);

  exit (status);
}

/* FIXME: this format loses with filenames containing newline.  */

static int
split_3 (s, u, binary, w)
     char *s, **u, **w;
     int *binary;
{
  size_t i;

#define ISWHITE(c) ((c) == ' ' || (c) == '\t')

  i = 0;
  while (ISWHITE (s[i]))
    ++i;

  /* The line has to be at least 35 characters long to contain correct
     message digest information.  */
  if (strlen (&s[i]) >= 35)
    {
      *u = &s[i];

      /* The first field has to be the 32-character hexadecimal
	 representation of the message digest.  If it not immediately
	 followed by a white space it's an error.  */
      i += 32;
      if (!ISWHITE (s[i]))
	return 1;

      s[i++] = '\0';

      if (s[i] != ' ' && s[i] != '*')
	return 1;
      *binary = s[i++] == '*';

      /* All characters between the type indicator and end of line are
	 significant -- that includes leading and trailing white space.  */
      *w = &s[i];

      /* So this line is valid as long as there is at least one character
	 for the filename.  */
      return (**w ? 0 : 1);
    }
  return 1;
}

/* FIXME: use strcspn.  */

static int
hex_digits (s)
     const char *s;
{
  while (*s)
    {
      if (!ISXDIGIT (*s))
        return 0;
      ++s;
    }
  return 1;
}

/* FIXME: allow newline in filename by encoding it. */

static int
md5_file (filename, binary, md5_result)
     const char *filename;
     int binary;
     unsigned char *md5_result;
{
  FILE *fp;
  int err;

  if (strcmp (filename, "-") == 0)
    {
      have_read_stdin = 1;
      fp = stdin;
    }
  else
    {
      /* OPENOPTS is a macro.  It varies with the system.
	 Some systems distinguish between internal and
	 external text representations.  */

      fp = fopen (filename, OPENOPTS (binary));
      if (fp == NULL)
	{
	  error (0, errno, "%s", filename);
	  return 1;
	}
    }

  err = md5_stream (fp, md5_result);
  if (err)
    {
      error (0, errno, "%s", filename);
      if (fp != stdin)
	fclose (fp);
      return 1;
    }

  if (fp != stdin && fclose (fp) == EOF)
    {
      error (0, errno, "%s", filename);
      return 1;
    }

  return 0;
}

static int
md5_check (checkfile_name, binary)
     const char *checkfile_name;
     int binary;
{
  FILE *checkfile_stream;
  int n_tests = 0;
  int n_tests_failed = 0;
  unsigned char md5buffer[16];
  size_t line_number;
  char *line;
  size_t line_chars_allocated;

  if (strcmp (checkfile_name, "-") == 0)
    {
      have_read_stdin = 1;
      checkfile_name = _("standard input");
      checkfile_stream = stdin;
    }
  else
    {
      checkfile_stream = fopen (checkfile_name, "r");
      if (checkfile_stream == NULL)
	{
	  error (0, errno, "%s", checkfile_name);
	  return 1;
	}
    }

  line_number = 0;
  line = NULL;
  line_chars_allocated = 0;
  do
    {
      char *filename;
      int type_flag;
      char *md5num;
      int err;
      int line_length;

      ++line_number;

      line_length = getline (&line, &line_chars_allocated, checkfile_stream);
      if (line_length <= 0)
	break;

      /* Ignore comment lines, which begin with a '#' character.  */
      if (line[0] == '#')
	continue;

      /* Remove any trailing newline.  */
      if (line[line_length - 1] == '\n')
	line[--line_length] = '\0';

      err = split_3 (line, &md5num, &type_flag, &filename);
      if (err || !hex_digits (md5num))
	{
	  if (verbose)
	    {
	      error (0, 0, _("%s: %lu: invalid MD5 checksum line"),
		     checkfile_name, (unsigned long) line_number);
	    }
	}
      else
	{
	  static const char bin2hex[] = { '0', '1', '2', '3',
					  '4', '5', '6', '7',
					  '8', '9', 'a', 'b',
					  'c', 'd', 'e', 'f' };
	  int fail;

	  ++n_tests;

	  fail = md5_file (filename, binary, md5buffer);

	  if (!fail)
	    {
	      size_t cnt;
	      /* Compare generated binary number with text representation
		 in check file.  Ignore case of hex digits.  */
	      for (cnt = 0; cnt < 16; ++cnt)
		{
		  if (TOLOWER (md5num[2 * cnt]) != bin2hex[md5buffer[cnt] >> 4]
		      || (TOLOWER (md5num[2 * cnt + 1])
			  != (bin2hex[md5buffer[cnt] & 0xf])))
		    break;
		}
	      if (cnt != 16)
		fail = 1;
	    }

	  if (fail)
	    ++n_tests_failed;

	  if (!quiet)
	    {
	      printf ("%s: %s\n", filename, (fail ? _("FAILED") : _("OK")));
	      fflush (stdout);
	    }
	}
    }
  while (!feof (checkfile_stream) && !ferror (checkfile_stream));

  /* FIXME: check ferror!!  */
  if (line)
    free (line);

  if (checkfile_stream != stdin && fclose (checkfile_stream) == EOF)
    {
      error (0, errno, "%s", checkfile_name);
      return 1;
    }

  if (!quiet)
    printf (n_tests == 1 ? (n_tests_failed ? _("Test failed\n")
					   : _("Test passed\n"))
			 : _("%d out of %d tests failed\n"),
	    n_tests_failed, n_tests);

  /* FIXME: warn if no tests are found?  */
  return (n_tests_failed == 0 ? 0 : 1);
}

int
main (argc, argv)
     int argc;
     char *argv[];
{
  unsigned char md5buffer[16];
  int do_check = 0;
  int do_help = 0;
  int do_version = 0;
  int opt;
  char **string = NULL;
  char n_strings = 0;
  size_t i;
  size_t err = 0;

  /* FIXME: comment. */
  /* Text is default of the Plumb/Lankester format.  */
  int binary = 0;

  /* Setting values of global variables.  */
  program_name = argv[0];

  while ((opt = getopt_long (argc, argv, "bchqs:tvV", long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case 0:			/* long option */
	break;
      case 'b':
	binary = 1;
	break;
      case 'c':
	do_check = 1;
	break;
      case 'h':
	do_help = 1;
	break;
      case 'q':
	quiet = 1;
	verbose = 0;
	break;
      case 's':
	{
	  if (string == NULL)
	    string = (char **) xmalloc ((argc - 1) * sizeof (char *));

	  if (optarg == NULL)
	    optarg = "";
	  string[n_strings++] = optarg;
	}
	break;
      case 't':
	binary = 0;
	break;
      case 'v':
	quiet = 0;
	verbose = 1;
	break;
      case 'V':
	do_version = 1;
	break;
      default:
	usage (EXIT_FAILURE);
      }

  if (do_version)
    {
      printf ("md5sum - %s\n", version_string);
      exit (EXIT_SUCCESS);
    }

  if (do_help)
    usage (EXIT_SUCCESS);

  if (n_strings > 0 && do_check != 0)
    {
      error (0, 0,
	     _("the --string and --check options are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (n_strings > 0)
    {
      /* --quiet does not make much sense with --string.  */
      if (optind < argc)
	{
	  error (0, 0, _("no files may be specified when using --string"));
	  usage (EXIT_FAILURE);
	}
      for (i = 0; i < n_strings; ++i)
	{
	  size_t cnt;
	  md5_buffer (string[i], strlen (string[i]), md5buffer);

	  for (cnt = 0; cnt < 16; ++cnt)
	    printf ("%02x", md5buffer[cnt]);

	  printf (" b \"%s\"\n", string[i]);
	}
    }
  else if (do_check == 0)
    {
      /* --quiet does no make much sense without --check.  So print the
	 result even if --quiet is given.  */
      if (optind == argc)
	argv[argc++] = "-";

      for (; optind < argc; ++optind)
	{
	  size_t i;
	  int fail;

	  fail = md5_file (argv[optind], binary, md5buffer);
	  err |= fail;
	  if (!fail)
	    {
	      for (i = 0; i < 16; ++i)
		printf ("%02x", md5buffer[i]);

	      printf (" %c%s\n", binary ? '*' : ' ', argv[optind]);
	    }
	}
    }
  else
    {
      if (optind + 1 < argc)
	{
	  error (0, 0,
		 _("only one argument may be specified when using --check"));
	  usage (EXIT_FAILURE);
	}

      err = md5_check ((optind == argc) ? "-" : argv[optind], binary);
    }

  if (fclose (stdout) == EOF)
    error (EXIT_FAILURE, errno, _("write error"));

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
