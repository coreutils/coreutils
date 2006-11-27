/* seq - print sequence of numbers to standard output.
   Copyright (C) 1994-2006 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Ulrich Drepper.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "c-strtod.h"
#include "error.h"
#include "quote.h"
#include "xstrtod.h"

/* Roll our own isfinite rather than using <math.h>, so that we don't
   have to worry about linking -lm just for isfinite.  */
#ifndef isfinite
# define isfinite(x) ((x) * 0 == 0)
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "seq"

#define AUTHORS "Ulrich Drepper"

/* If true print all number with equal width.  */
static bool equal_width;

/* The name that this program was run with.  */
char *program_name;

/* The string used to separate two numbers.  */
static char const *separator;

/* The string output after all numbers have been output.
   Usually "\n" or "\0".  */
/* FIXME: make this an option.  */
static char const terminator[] = "\n";

static struct option const long_options[] =
{
  { "equal-width", no_argument, NULL, 'w'},
  { "format", required_argument, NULL, 'f'},
  { "separator", required_argument, NULL, 's'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  { NULL, 0, NULL, 0}
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
Usage: %s [OPTION]... LAST\n\
  or:  %s [OPTION]... FIRST LAST\n\
  or:  %s [OPTION]... FIRST INCREMENT LAST\n\
"), program_name, program_name, program_name);
      fputs (_("\
Print numbers from FIRST to LAST, in steps of INCREMENT.\n\
\n\
  -f, --format=FORMAT      use printf style floating-point FORMAT\n\
  -s, --separator=STRING   use STRING to separate numbers (default: \\n)\n\
  -w, --equal-width        equalize width by padding with leading zeroes\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
If FIRST or INCREMENT is omitted, it defaults to 1.  That is, an\n\
omitted INCREMENT defaults to 1 even when LAST is smaller than FIRST.\n\
FIRST, INCREMENT, and LAST are interpreted as floating point values.\n\
INCREMENT is usually positive if FIRST is smaller than LAST, and\n\
INCREMENT is usually negative if FIRST is greater than LAST.\n\
"), stdout);
      fputs (_("\
FORMAT must be suitable for printing one argument of type `double';\n\
it defaults to %.PRECf if FIRST, INCREMENT, and LAST are all fixed point\n\
decimal numbers with maximum precision PREC, and to %g otherwise.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* A command-line operand.  */
struct operand
{
  /* Its value, converted to 'long double'.  */
  long double value;

  /* Its print width, if it were printed out in a form similar to its
     input form.  An input like "-.1" is treated like "-0.1", and an
     input like "1." is treated like "1", but otherwise widths are
     left alone.  */
  size_t width;

  /* Number of digits after the decimal point, or INT_MAX if the
     number can't easily be expressed as a fixed-point number.  */
  int precision;
};
typedef struct operand operand;

/* Read a long double value from the command line.
   Return if the string is correct else signal error.  */

static operand
scan_arg (const char *arg)
{
  operand ret;

  if (! xstrtold (arg, NULL, &ret.value, c_strtold))
    {
      error (0, 0, _("invalid floating point argument: %s"), arg);
      usage (EXIT_FAILURE);
    }

  ret.width = strlen (arg);
  ret.precision = INT_MAX;

  if (! arg[strcspn (arg, "eExX")] && isfinite (ret.value))
    {
      char const *decimal_point = strchr (arg, '.');
      if (! decimal_point)
	ret.precision = 0;
      else
	{
	  size_t fraction_len = strlen (decimal_point + 1);
	  if (fraction_len <= INT_MAX)
	    ret.precision = fraction_len;
	  ret.width += (fraction_len == 0
			? -1
			: (decimal_point == arg
			   || ! ISDIGIT (decimal_point[-1])));
	}
    }

  return ret;
}

/* If FORMAT is a valid printf format for a double argument, return
   its long double equivalent, possibly allocated from dynamic
   storage; otherwise, return NULL.  */

static char const *
long_double_format (char const *fmt)
{
  size_t i;
  size_t prefix_len;
  bool has_L;

  for (i = 0; ! (fmt[i] == '%' && fmt[i + 1] != '%'); i++)
    if (! fmt[i])
      return NULL;

  i++;
  i += strspn (fmt + i, "-+#0 '");
  i += strspn (fmt + i, "0123456789");
  if (fmt[i] == '.')
    {
      i++;
      i += strspn (fmt + i, "0123456789");
    }

  prefix_len = i;
  has_L = (fmt[i] == 'L');
  i += has_L;
  if (! strchr ("efgaEFGA", fmt[i]))
    return NULL;

  for (i++; ! (fmt[i] == '%' && fmt[i + 1] != '%'); i++)
    if (! fmt[i])
      {
	size_t format_size = i + 1;
	char *ldfmt = xmalloc (format_size + 1);
	memcpy (ldfmt, fmt, prefix_len);
	ldfmt[prefix_len] = 'L';
	strcpy (ldfmt + prefix_len + 1, fmt + prefix_len + has_L);
	return ldfmt;
      }

  return NULL;
}

/* Actually print the sequence of numbers in the specified range, with the
   given or default stepping and format.  */

static void
print_numbers (char const *fmt,
	       long double first, long double step, long double last)
{
  long double i;

  for (i = 0; /* empty */; i++)
    {
      long double x = first + i * step;
      if (step < 0 ? x < last : last < x)
	break;
      if (i)
	fputs (separator, stdout);
      printf (fmt, x);
    }

  if (i)
    fputs (terminator, stdout);
}

/* Return the default format given FIRST, STEP, and LAST.  */
static char const *
get_default_format (operand first, operand step, operand last)
{
  static char format_buf[sizeof "%0.Lf" + 2 * INT_STRLEN_BOUND (int)];

  int prec = MAX (first.precision, step.precision);

  if (prec != INT_MAX && last.precision != INT_MAX)
    {
      if (equal_width)
	{
	  size_t first_width = first.width + (prec - first.precision);
	  size_t last_width = last.width + (prec - last.precision);
	  if (first.width <= first_width
	      && (last.width < last_width) == (prec < last.precision))
	    {
	      size_t width = MAX (first_width, last_width);
	      if (width <= INT_MAX)
		{
		  int w = width;
		  sprintf (format_buf, "%%0%d.%dLf", w, prec);
		  return format_buf;
		}
	    }
	}
      else
	{
	  sprintf (format_buf, "%%.%dLf", prec);
	  return format_buf;
	}
    }

  return "%Lg";
}

int
main (int argc, char **argv)
{
  int optc;
  operand first = { 1, 1, 0 };
  operand step = { 1, 1, 0 };
  operand last;

  /* The printf(3) format used for output.  */
  char const *format_str = NULL;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  equal_width = false;
  separator = "\n";

  /* We have to handle negative numbers in the command line but this
     conflicts with the command line arguments.  So explicitly check first
     whether the next argument looks like a negative number.  */
  while (optind < argc)
    {
      if (argv[optind][0] == '-'
	  && ((optc = argv[optind][1]) == '.' || ISDIGIT (optc)))
	{
	  /* means negative number */
	  break;
	}

      optc = getopt_long (argc, argv, "+f:s:w", long_options, NULL);
      if (optc == -1)
	break;

      switch (optc)
	{
	case 'f':
	  format_str = optarg;
	  break;

	case 's':
	  separator = optarg;
	  break;

	case 'w':
	  equal_width = true;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc - optind < 1)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  if (3 < argc - optind)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind + 3]));
      usage (EXIT_FAILURE);
    }

  if (format_str)
    {
      char const *f = long_double_format (format_str);
      if (! f)
	{
	  error (0, 0, _("invalid format string: %s"), quote (format_str));
	  usage (EXIT_FAILURE);
	}
      format_str = f;
    }

  last = scan_arg (argv[optind++]);

  if (optind < argc)
    {
      first = last;
      last = scan_arg (argv[optind++]);

      if (optind < argc)
	{
	  step = last;
	  last = scan_arg (argv[optind++]);
	}
    }

  if (format_str != NULL && equal_width)
    {
      error (0, 0, _("\
format string may not be specified when printing equal width strings"));
      usage (EXIT_FAILURE);
    }

  if (format_str == NULL)
    format_str = get_default_format (first, step, last);

  print_numbers (format_str, first.value, step.value, last.value);

  exit (EXIT_SUCCESS);
}
