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

#include "long-options.h"
#include "md5.h"
#include "getline.h"
#include "system.h"
#include "error.h"

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

/* With --check, don't generate any output.
   The exit code indicates success or failure.  */
static int status_only = 0;

/* With --check, print a message to standard error warning about each
   improperly formatted MD5 checksum line.  */
static int warn = 0;

/* The name this program was run with.  */
char *program_name;

static const struct option long_options[] =
{
  { "binary", no_argument, 0, 'b' },
  { "check", no_argument, 0, 'c' },
  { "status", no_argument, 0, 2 },
  { "string", required_argument, 0, 1 },
  { "text", no_argument, 0, 't' },
  { "warn", no_argument, 0, 'w' },
  { NULL, 0, NULL, 0 }
};

char *xmalloc ();

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    printf (_("\
Usage: %s [OPTION] [FILE]...\n\
  or:  %s [OPTION] --check [FILE]\n\
  or:  %s [OPTION] --string=STRING ...\n\
Print or check MD5 checksums.\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
  -b, --binary            read files in binary mode\n\
  -t, --text              read files in text mode (default)\n\
  -c, --check             check MD5 sums against given list\n\
\n\
The following two options are useful only when verifying checksums:\n\
      --status            don't output anything, status code shows success\n\
  -w, --warn              warn about improperly formated MD5 checksum lines\n\
\n\
      --string=STRING     compute checksum for STRING\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
\n\
The sums are computed as described in RFC 1321.  When checking, the input\n\
should be a former output of this program.  The default mode is to print\n\
a line with checksum, a character indicating type (`*' for binary, ` ' for\n\
text), and name for each FILE.\n"),
	    program_name, program_name, program_name);

  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* FIXME: this format loses with filenames containing newline.  */

static int
split_3 (char *s, char **u, int *binary, char **w)
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

static int
hex_digits (const char *s)
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
md5_file (const char *filename, int binary, unsigned char *md5_result)
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
md5_check (const char *checkfile_name, int binary)
{
  FILE *checkfile_stream;
  int n_properly_formated_lines = 0;
  int n_mismatched_checksums = 0;
  int n_open_or_read_failures = 0;
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
	  if (warn)
	    {
	      error (0, 0,
		     _("%s: %lu: improperly formatted MD5 checksum line"),
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

	  ++n_properly_formated_lines;

	  fail = md5_file (filename, binary, md5buffer);

	  if (fail)
	    {
	      ++n_open_or_read_failures;
	      if (!status_only)
		{
		  printf (_("%s: FAILED open or read\n"), filename);
		  fflush (stdout);
		}
	    }
	  else
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
		++n_mismatched_checksums;

	      if (!status_only)
		{
		  printf ("%s: %s\n", filename,
			  (cnt != 16 ? _("FAILED") : _("OK")));
		  fflush (stdout);
		}
	    }
	}
    }
  while (!feof (checkfile_stream) && !ferror (checkfile_stream));

  if (line)
    free (line);

  if (ferror (checkfile_stream))
    {
      error (0, 0, _("%s: read error"), checkfile_name);
      return 1;
    }

  if (checkfile_stream != stdin && fclose (checkfile_stream) == EOF)
    {
      error (0, errno, "%s", checkfile_name);
      return 1;
    }

  if (n_properly_formated_lines == 0)
    {
      /* Warn if no tests are found.  */
      error (0, 0, _("%s: no properly formatted MD5 checksum lines found"),
	     checkfile_name);
    }
  else
    {
      if (!status_only)
	{
	  int n_computed_checkums = (n_properly_formated_lines
				     - n_open_or_read_failures);

	  if (n_open_or_read_failures > 0)
	    {
	      error (0, 0,
		   _("WARNING: %d of %d listed file%s could not be read\n"),
		     n_open_or_read_failures, n_properly_formated_lines,
		     (n_properly_formated_lines == 1 ? "" : "s"));
	    }

	  if (n_mismatched_checksums > 0)
	    {
	      error (0, 0,
		   _("WARNING: %d of %d computed checksum%s did NOT match"),
		     n_mismatched_checksums, n_computed_checkums,
		     (n_computed_checkums == 1 ? "" : "s"));
	    }
	}
    }

  return ((n_properly_formated_lines > 0 && n_mismatched_checksums == 0
	   && n_open_or_read_failures == 0) ? 0 : 1);
}

int
main (int argc, char **argv)
{
  unsigned char md5buffer[16];
  int do_check = 0;
  int do_version = 0;
  int opt;
  char **string = NULL;
  size_t n_strings = 0;
  size_t i;
  size_t err = 0;

  /* Text is default of the Plumb/Lankester format.  */
  int binary = 0;

  /* Setting values of global variables.  */
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, "md5sum", PACKAGE_VERSION, usage);

  while ((opt = getopt_long (argc, argv, "bctw", long_options, NULL))
	 != EOF)
    switch (opt)
      {
      case 0:			/* long option */
	break;
      case 1: /* --string */
	{
	  if (string == NULL)
	    string = (char **) xmalloc ((argc - 1) * sizeof (char *));

	  if (optarg == NULL)
	    optarg = "";
	  string[n_strings++] = optarg;
	}
	break;
      case 'b':
	binary = 1;
	break;
      case 'c':
	do_check = 1;
	break;
      case 2:
	status_only = 1;
	warn = 0;
	break;
      case 't':
	binary = 0;
	break;
      case 'w':
	status_only = 0;
	warn = 1;
	break;
      default:
	usage (EXIT_FAILURE);
      }

  if (do_version)
    {
      printf ("md5sum - %s\n", PACKAGE_VERSION);
      exit (EXIT_SUCCESS);
    }

  if (n_strings > 0 && do_check)
    {
      error (0, 0,
	     _("the --string and --check options are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (status_only && !do_check)
    {
      error (0, 0,
       _("the --status option is meaningful only when verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (warn && !do_check)
    {
      error (0, 0,
       _("the --warn option is meaningful only when verifying checksums"));
      usage (EXIT_FAILURE);
    }

  if (n_strings > 0)
    {
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

	  printf ("  \"%s\"\n", string[i]);
	}
    }
  else if (do_check)
    {
      if (optind + 1 < argc)
	{
	  error (0, 0,
		 _("only one argument may be specified when using --check"));
	  usage (EXIT_FAILURE);
	}

      err = md5_check ((optind == argc) ? "-" : argv[optind], binary);
    }
  else
    {
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

  if (fclose (stdout) == EOF)
    error (EXIT_FAILURE, errno, _("write error"));

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
