/* Compute MD5 or SHA1 checksum of files or strings
   Copyright (C) 1995-2002 Free Software Foundation, Inc.

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

#include <config.h>

#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"

#include "md5.h"
#include "sha.h"
#include "checksum.h"
#include "getline.h"
#include "closeout.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME (algorithm == ALG_MD5 ? "md5sum" : "shasum")

#define AUTHORS N_ ("Ulrich Drepper and Scott Miller")

/* Most systems do not distinguish between external and internal
   text representations.  */
/* FIXME: This begs for an autoconf test.  */
#if O_BINARY
# define OPENOPTS(BINARY) ((BINARY) != 0 ? TEXT1TO1 : TEXTCNVT)
# define TEXT1TO1 "rb"
# define TEXTCNVT "r"
#else
# if defined VMS
#  define OPENOPTS(BINARY) ((BINARY) != 0 ? TEXT1TO1 : TEXTCNVT)
#  define TEXT1TO1 "rb", "ctx=stm"
#  define TEXTCNVT "r", "ctx=stm"
# else
#  if UNIX || __UNIX__ || unix || __unix__ || _POSIX_VERSION
#   define OPENOPTS(BINARY) "r"
#  else
    /* The following line is intended to evoke an error.
       Using #error is not portable enough.  */
    "Cannot determine system type."
#  endif
# endif
#endif


#define DIGEST_TYPE_STRING(Alg) ((Alg) == ALG_MD5 ? "MD5" : "SHA1")
#define DIGEST_STREAM(Alg) ((Alg) == ALG_MD5 ? md5_stream : sha_stream)

#define DIGEST_BITS(Alg) ((Alg) == ALG_MD5 ? 128 : 160)
#define DIGEST_HEX_BYTES(Alg) (DIGEST_BITS (Alg) / 4)
#define DIGEST_BIN_BYTES(Alg) (DIGEST_BITS (Alg) / 8)

#define MAX_DIGEST_BIN_BYTES MAX (DIGEST_BIN_BYTES (ALG_MD5), \
				  DIGEST_BIN_BYTES (ALG_SHA1))

/* The minimum length of a valid digest line.  This length does
   not include any newline character at the end of a line.  */
#define MIN_DIGEST_LINE_LENGTH(Alg) \
  (DIGEST_HEX_BYTES (Alg) /* length of hexadecimal message digest */ \
   + 2 /* blank and binary indicator */ \
   + 1 /* minimum filename length */ )

/* Nonzero if any of the files read were the standard input. */
static int have_read_stdin;

/* The minimum length of a valid checksum line for the selected algorithm.  */
static size_t min_digest_line_length;

/* Set to the length of a digest hex string for the selected algorithm.  */
static size_t digest_hex_bytes;

/* With --check, don't generate any output.
   The exit code indicates success or failure.  */
static int status_only = 0;

/* With --check, print a message to standard error warning about each
   improperly formatted checksum line.  */
static int warn = 0;

/* Declared and set via one of the wrapper .c files.  */
/* int algorithm = ALG_UNSPECIFIED; */

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
  { GETOPT_HELP_OPTION_DECL },
  { GETOPT_VERSION_OPTION_DECL },
  { NULL, 0, NULL, 0 }
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
Usage: %s [OPTION] [FILE]...\n\
  or:  %s [OPTION] --check [FILE]\n\
Print or check %s (%d-bit) checksums.\n\
With no FILE, or when FILE is -, read standard input.\n\
"),
	      program_name, program_name,
	      DIGEST_TYPE_STRING (algorithm),
	      DIGEST_BITS (algorithm));
      printf (_("\
\n\
  -b, --binary            read files in binary mode (default on DOS/Windows)\n\
  -c, --check             check %s sums against given list\n\
  -t, --text              read files in text mode (default)\n\
\n\
"),
	      DIGEST_TYPE_STRING (algorithm));
      fputs (_("\
The following two options are useful only when verifying checksums:\n\
      --status            don't output anything, status code shows success\n\
  -w, --warn              warn about improperly formated checksum lines\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\
\n\
The sums are computed as described in %s.  When checking, the input\n\
should be a former output of this program.  The default mode is to print\n\
a line with checksum, a character indicating type (`*' for binary, ` ' for\n\
text), and name for each FILE.\n"),
	      (algorithm == ALG_MD5 ? "RFC 1321" : "FIPS-180-1"));
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }

  exit (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Split the string S (of length S_LEN) into three parts:
   a hexadecimal digest, binary flag, and the file name.
   S is modified.  */

static int
split_3 (char *s, size_t s_len,
	 unsigned char **hex_digest, int *binary, char **file_name)
{
  size_t i;
  int escaped_filename = 0;

#define ISWHITE(c) ((c) == ' ' || (c) == '\t')

  i = 0;
  while (ISWHITE (s[i]))
    ++i;

  /* Ignore this line if it is too short.
     Each line must have at least `min_digest_line_length - 1' (or one more, if
     the first is a backslash) more characters to contain correct message digest
     information.  */
  if (s_len - i < min_digest_line_length + (s[i] == '\\'))
    return 1;

  if (s[i] == '\\')
    {
      ++i;
      escaped_filename = 1;
    }
  *hex_digest = (unsigned char *) &s[i];

  /* The first field has to be the n-character hexadecimal
     representation of the message digest.  If it is not followed
     immediately by a white space it's an error.  */
  i += digest_hex_bytes;
  if (!ISWHITE (s[i]))
    return 1;

  s[i++] = '\0';

  if (s[i] != ' ' && s[i] != '*')
    return 1;
  *binary = (s[i++] == '*');

  /* All characters between the type indicator and end of line are
     significant -- that includes leading and trailing white space.  */
  *file_name = &s[i];

  if (escaped_filename)
    {
      /* Translate each `\n' string in the file name to a NEWLINE,
	 and each `\\' string to a backslash.  */

      char *dst = &s[i];

      while (i < s_len)
	{
	  switch (s[i])
	    {
	    case '\\':
	      if (i == s_len - 1)
		{
		  /* A valid line does not end with a backslash.  */
		  return 1;
		}
	      ++i;
	      switch (s[i++])
		{
		case 'n':
		  *dst++ = '\n';
		  break;
		case '\\':
		  *dst++ = '\\';
		  break;
		default:
		  /* Only `\' or `n' may follow a backslash.  */
		  return 1;
		}
	      break;

	    case '\0':
	      /* The file name may not contain a NUL.  */
	      return 1;
	      break;

	    default:
	      *dst++ = s[i++];
	      break;
	    }
	}
      *dst = '\0';
    }
  return 0;
}

static int
hex_digits (unsigned char const *s)
{
  while (*s)
    {
      if (!ISXDIGIT (*s))
        return 0;
      ++s;
    }
  return 1;
}

/* An interface to the function, DIGEST_STREAM,
   (either md5_stream or sha_stream).
   Operate on FILENAME (it may be "-") and put the result in *BIN_RESULT.
   Return non-zero upon failure, zero to indicate success.  */

static int
digest_file (const char *filename, int binary, unsigned char *bin_result,
	   int (*digest_stream)(FILE *, void *))
{
  FILE *fp;
  int err;

  if (STREQ (filename, "-"))
    {
      have_read_stdin = 1;
      fp = stdin;
#if O_BINARY
      /* If we need binary reads from a pipe or redirected stdin, we need
	 to switch it to BINARY mode here, since stdin is already open.  */
      if (binary)
	SET_BINARY (fileno (stdin));
#endif
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

  err = (*digest_stream) (fp, bin_result);
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
digest_check (const char *checkfile_name, int (*digest_stream)(FILE *, void *))
{
  FILE *checkfile_stream;
  int n_properly_formated_lines = 0;
  int n_mismatched_checksums = 0;
  int n_open_or_read_failures = 0;
  unsigned char bin_buffer[MAX_DIGEST_BIN_BYTES];
  size_t line_number;
  char *line;
  size_t line_chars_allocated;

  if (STREQ (checkfile_name, "-"))
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

  SET_MODE (fileno (checkfile_stream), O_TEXT);
  line_number = 0;
  line = NULL;
  line_chars_allocated = 0;
  do
    {
      char *filename;
      int binary;
      unsigned char *hex_digest;
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

      err = split_3 (line, line_length, &hex_digest, &binary, &filename);
      if (err || !hex_digits (hex_digest))
	{
	  if (warn)
	    {
	      error (0, 0,
		     _("%s: %lu: improperly formatted %s checksum line"),
		     checkfile_name, (unsigned long) line_number,
		     DIGEST_TYPE_STRING (algorithm));
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

	  fail = digest_file (filename, binary, bin_buffer, digest_stream);

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
	      size_t digest_bin_bytes = digest_hex_bytes / 2;
	      size_t cnt;
	      /* Compare generated binary number with text representation
		 in check file.  Ignore case of hex digits.  */
	      for (cnt = 0; cnt < digest_bin_bytes; ++cnt)
		{
		  if (TOLOWER (hex_digest[2 * cnt])
		      != bin2hex[bin_buffer[cnt] >> 4]
		      || (TOLOWER (hex_digest[2 * cnt + 1])
			  != (bin2hex[bin_buffer[cnt] & 0xf])))
		    break;
		}
	      if (cnt != digest_bin_bytes)
		++n_mismatched_checksums;

	      if (!status_only)
		{
		  printf ("%s: %s\n", filename,
			  (cnt != digest_bin_bytes ? _("FAILED") : _("OK")));
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
      error (0, 0, _("%s: no properly formatted %s checksum lines found"),
	     checkfile_name, DIGEST_TYPE_STRING (algorithm));
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
		     _("WARNING: %d of %d listed %s could not be read"),
		     n_open_or_read_failures, n_properly_formated_lines,
		     (n_properly_formated_lines == 1
		      ? _("file") : _("files")));
	    }

	  if (n_mismatched_checksums > 0)
	    {
	      error (0, 0,
		   _("WARNING: %d of %d computed %s did NOT match"),
		     n_mismatched_checksums, n_computed_checkums,
		     (n_computed_checkums == 1
		      ? _("checksum") : _("checksums")));
	    }
	}
    }

  return ((n_properly_formated_lines > 0 && n_mismatched_checksums == 0
	   && n_open_or_read_failures == 0) ? 0 : 1);
}

int
main (int argc, char **argv)
{
  unsigned char bin_buffer[MAX_DIGEST_BIN_BYTES];
  int do_check = 0;
  int opt;
  char **string = NULL;
  size_t n_strings = 0;
  size_t err = 0;
  int file_type_specified = 0;

#if O_BINARY
  /* Binary is default on MSDOS, so the actual file contents
     are used in computation.  */
  int binary = 1;
#else
  /* Text is default of the Plumb/Lankester format.  */
  int binary = 0;
#endif

  /* Setting values of global variables.  */
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((opt = getopt_long (argc, argv, "bctw", long_options, NULL)) != -1)
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
	file_type_specified = 1;
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
	file_type_specified = 1;
	binary = 0;
	break;
      case 'w':
	status_only = 0;
	warn = 1;
	break;
      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
	usage (EXIT_FAILURE);
      }

  min_digest_line_length = MIN_DIGEST_LINE_LENGTH (algorithm);
  digest_hex_bytes = DIGEST_HEX_BYTES (algorithm);

  if (file_type_specified && do_check)
    {
      error (0, 0, _("the --binary and --text options are meaningless when \
verifying checksums"));
      usage (EXIT_FAILURE);
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
      size_t i;

      if (optind < argc)
	{
	  error (0, 0, _("no files may be specified when using --string"));
	  usage (EXIT_FAILURE);
	}
      for (i = 0; i < n_strings; ++i)
	{
	  size_t cnt;
	  if (algorithm == ALG_MD5)
	    md5_buffer (string[i], strlen (string[i]), bin_buffer);
	  else
	    sha_buffer (string[i], strlen (string[i]), bin_buffer);

	  for (cnt = 0; cnt < (digest_hex_bytes / 2); ++cnt)
	    printf ("%02x", bin_buffer[cnt]);

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

      err = digest_check ((optind == argc) ? "-" : argv[optind],
			  DIGEST_STREAM (algorithm));
    }
  else
    {
      if (optind == argc)
	argv[argc++] = "-";

      for (; optind < argc; ++optind)
	{
	  int fail;
	  char *file = argv[optind];

	  fail = digest_file (file, binary, bin_buffer,
			      DIGEST_STREAM (algorithm));
	  err |= fail;
	  if (!fail)
	    {
	      size_t i;

	      /* Output a leading backslash if the file name contains
		 a newline or backslash.  */
	      if (strchr (file, '\n') || strchr (file, '\\'))
		putchar ('\\');

	      for (i = 0; i < (digest_hex_bytes / 2); ++i)
		printf ("%02x", bin_buffer[i]);

	      putchar (' ');
	      if (binary)
		putchar ('*');
	      else
		putchar (' ');

	      /* Translate each NEWLINE byte to the string, "\\n",
		 and each backslash to "\\\\".  */
	      for (i = 0; i < strlen (file); ++i)
		{
		  switch (file[i])
		    {
		    case '\n':
		      fputs ("\\n", stdout);
		      break;

		    case '\\':
		      fputs ("\\\\", stdout);
		      break;

		    default:
		      putchar (file[i]);
		      break;
		    }
		}
	      putchar ('\n');
	    }
	}
    }

  if (have_read_stdin && fclose (stdin) == EOF)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (err == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
