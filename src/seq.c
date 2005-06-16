/* seq - print sequence of numbers to standard output.
   Copyright (C) 1994-2005 Free Software Foundation, Inc.

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
#include <math.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "c-strtod.h"
#include "error.h"
#include "quote.h"
#include "xstrtol.h"
#include "xstrtod.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "seq"

#define AUTHORS "Ulrich Drepper"

/* If true print all number with equal width.  */
static bool equal_width;

/* The name that this program was run with.  */
char *program_name;

/* The string used to separate two numbers.  */
static char *separator;

/* The string output after all numbers have been output.
   Usually "\n" or "\0".  */
/* FIXME: make this an option.  */
static char *terminator = "\n";

/* The representation of the decimal point in the current locale.  */
static char decimal_point;

/* The starting number.  */
static double first;

/* The increment.  */
static double step = 1.0;

/* The last number.  */
static double last;

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
  -f, --format=FORMAT      use printf style floating-point FORMAT (default: %g)\n\
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
When given, the FORMAT argument must contain exactly one of\n\
the printf-style, floating point output formats %e, %f, %g\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Read a double value from the command line.
   Return if the string is correct else signal error.  */

static double
scan_double_arg (const char *arg)
{
  double ret_val;

  if (! xstrtod (arg, NULL, &ret_val, c_strtod))
    {
      error (0, 0, _("invalid floating point argument: %s"), arg);
      usage (EXIT_FAILURE);
    }

  return ret_val;
}

/* Return true if the format string is valid for a single `double'
   argument.  */

static bool
valid_format (const char *fmt)
{
  while (*fmt != '\0')
    {
      if (*fmt == '%')
	{
	  fmt++;
	  if (*fmt != '%')
	    break;
	}

      fmt++;
    }
  if (*fmt == '\0')
    return false;

  fmt += strspn (fmt, "-+#0 '");
  if (ISDIGIT (*fmt) || *fmt == '.')
    {
      fmt += strspn (fmt, "0123456789");

      if (*fmt == '.')
	{
	  ++fmt;
	  fmt += strspn (fmt, "0123456789");
	}
    }

  if (!(*fmt == 'e' || *fmt == 'f' || *fmt == 'g'))
    return false;

  fmt++;
  while (*fmt != '\0')
    {
      if (*fmt == '%')
	{
	  fmt++;
	  if (*fmt != '%')
	    return false;
	}

      fmt++;
    }

  return true;
}

/* Actually print the sequence of numbers in the specified range, with the
   given or default stepping and format.  */
static void
print_numbers (const char *fmt)
{
  double i;

  for (i = 0; /* empty */; i++)
    {
      double x = first + i * step;
      if (step < 0 ? x < last : last < x)
	break;
      if (i)
	fputs (separator, stdout);
      printf (fmt, x);
    }

  if (i)
    fputs (terminator, stdout);
}

#if HAVE_RINT && HAVE_MODF && HAVE_FLOOR

/* Return a printf-style format string with which all selected numbers
   will format to strings of the same width.  */

static char *
get_width_format (void)
{
  static char buffer[256];
  int full_width;
  int frac_width;
  int width1, width2;
  double max_val;
  double min_val;
  double temp;

  if (first > last)
    {
      min_val = first - step * floor ((first - last) / step);
      max_val = first;
    }
  else
    {
      min_val = first;
      max_val = first + step * floor ((last - first) / step);
    }

  sprintf (buffer, "%g", rint (max_val));
  if (buffer[strspn (buffer, "-0123456789")] != '\0')
    return "%g";
  width1 = strlen (buffer);

  if (min_val < 0.0)
    {
      double int_min_val = rint (min_val);
      sprintf (buffer, "%g", int_min_val);
      if (buffer[strspn (buffer, "-0123456789")] != '\0')
	return "%g";
      /* On some systems, `seq -w -.1 .1 .1' results in buffer being `-0'.
	 On others, it is just `0'.  The former results in better output.  */
      width2 = (int_min_val == 0 ? 2 : strlen (buffer));

      width1 = width1 > width2 ? width1 : width2;
    }
  full_width = width1;

  sprintf (buffer, "%g", 1.0 + modf (fabs (min_val), &temp));
  width1 = strlen (buffer);
  if (width1 == 1)
    width1 = 0;
  else
    {
      if (buffer[0] != '1'
	  || buffer[1] != decimal_point
	  || buffer[2 + strspn (&buffer[2], "0123456789")] != '\0')
	return "%g";
      width1 -= 2;
    }

  sprintf (buffer, "%g", 1.0 + modf (fabs (step), &temp));
  width2 = strlen (buffer);
  if (width2 == 1)
    width2 = 0;
  else
    {
      if (buffer[0] != '1'
	  || buffer[1] != decimal_point
	  || buffer[2 + strspn (&buffer[2], "0123456789")] != '\0')
	return "%g";
      width2 -= 2;
    }
  frac_width = width1 > width2 ? width1 : width2;

  if (frac_width)
    sprintf (buffer, "%%0%d.%df", full_width + 1 + frac_width, frac_width);
  else
    sprintf (buffer, "%%0%dg", full_width);

  return buffer;
}

#else	/* one of the math functions rint, modf, floor is missing.  */

static char *
get_width_format (void)
{
  /* We cannot compute the needed information to determine the correct
     answer.  So we simply return a value that works for all cases.  */
  return "%g";
}

#endif

int
main (int argc, char **argv)
{
  int optc;

  /* The printf(3) format used for output.  */
  char *format_str = NULL;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  equal_width = false;
  separator = "\n";
  first = 1.0;

  /* Get locale's representation of the decimal point.  */
  {
    struct lconv const *locale = localeconv ();

    /* If the locale doesn't define a decimal point, or if the decimal
       point is multibyte, use the C locale's decimal point.  FIXME:
       add support for multibyte decimal points.  */
    decimal_point = locale->decimal_point[0];
    if (! decimal_point || locale->decimal_point[1])
      decimal_point = '.';
  }

  /* We have to handle negative numbers in the command line but this
     conflicts with the command line arguments.  So explicitly check first
     whether the next argument looks like a negative number.  */
  while (optind < argc)
    {
      if (argv[optind][0] == '-'
	  && ((optc = argv[optind][1]) == decimal_point
	      || ISDIGIT (optc)))
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

  if (format_str && !valid_format (format_str))
    {
      error (0, 0, _("invalid format string: %s"), quote (format_str));
      usage (EXIT_FAILURE);
    }

  last = scan_double_arg (argv[optind++]);

  if (optind < argc)
    {
      first = last;
      last = scan_double_arg (argv[optind++]);

      if (optind < argc)
	{
	  step = last;
	  last = scan_double_arg (argv[optind++]);
	}
    }

  if (format_str != NULL && equal_width)
    {
      error (0, 0, _("\
format string may not be specified when printing equal width strings"));
      usage (EXIT_FAILURE);
    }

  if (format_str == NULL)
    {
      if (equal_width)
	format_str = get_width_format ();
      else
	format_str = "%g";
    }

  print_numbers (format_str);

  exit (EXIT_SUCCESS);
}
