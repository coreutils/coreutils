/* seq - print sequence of numbers to standard output.
   Copyright (C) 1994-1999 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper.  */

#include <config.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "error.h"
#include "xstrtod.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "seq"

#define AUTHORS "Ulrich Drepper"

static double scan_double_arg PARAMS ((const char *arg));
static int check_format PARAMS ((const char *format_string));
static char *get_width_format PARAMS ((void));
static int print_numbers PARAMS ((const char *format_str));

/* If nonzero print all number with equal width.  */
static int equal_width;

/* The printf(3) format used for output.  */
static char *format_str;

/* The starting number.  */
static double first;

/* The name that this program was run with.  */
char *program_name;

/* The string used to separate two numbers.  */
static char *separator;

/* The string output after all numbers have been output.
   Usually "\n" or "\0".  */
/* FIXME: make this an option.  */
static char *terminator = "\n";

/* The representation of the decimal point in the current locale.
   Always "." if the localeconv function is not supported.  */
static char *decimal_point = ".";

/* The increment.  */
static double step;

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
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... LAST\n\
  or:  %s [OPTION]... FIRST LAST\n\
  or:  %s [OPTION]... FIRST INCREMENT LAST\n\
"), program_name, program_name, program_name);
      printf (_("\
Print numbers from FIRST to LAST, in steps of INCREMENT.\n\
\n\
  -f, --format FORMAT      use printf(3) style FORMAT (default: %%g)\n\
  -s, --separator STRING   use STRING to separate numbers (default: \\n)\n\
  -w, --equal-width        equalize width by padding with leading zeroes\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
\n\
If FIRST or INCREMENT is omitted, it defaults to 1.\n\
FIRST, INCREMENT, and LAST are interpreted as floating point values.\n\
INCREMENT should be positive if FIRST is smaller than LAST, and negative\n\
otherwise.  When given, the FORMAT argument must contain exactly one of\n\
the printf-style, floating point output formats %%e, %%f, or %%g.\n\
"));
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int errs;
  int optc;
  int step_is_set;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  equal_width = 0;
  format_str = NULL;
  separator = "\n";
  first = 1.0;
  step_is_set = 0;

  /* Figure out the locale's idea of a decimal point.  */
#ifdef HAVE_LOCALECONV
  {
    struct lconv *locale;

    locale = localeconv ();
    /* Paranoia.  */
    if (locale && locale->decimal_point && locale->decimal_point[0] != '\0')
      decimal_point = locale->decimal_point;
  }
#endif

  /* We have to handle negative numbers in the command line but this
     conflicts with the command line arguments.  So explicitly check first
     whether the next argument looks like a negative number.  */
  while (optind < argc)
    {
      if (argv[optind][0] == '-'
	  && ((optc = argv[optind][1]) == decimal_point[0]
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
	case 0:
	  break;

	case 'f':
	  format_str = optarg;
	  break;

	case 's':
	  separator = optarg;
	  break;

	case 'w':
	  equal_width = 1;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (1);
	  /* NOTREACHED */
	}
    }

  if (optind >= argc)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
      /* NOTREACHED */
    }
  last = scan_double_arg (argv[optind++]);

  if (optind < argc)
    {
      first = last;
      last = scan_double_arg (argv[optind++]);

      if (optind < argc)
	{
	  step = last;
	  step_is_set = 1;
	  last = scan_double_arg (argv[optind++]);

	  if (optind < argc)
	    {
	      usage (1);
	      /* NOTREACHED */
	    }
	}
    }

  if (format_str != NULL && equal_width)
    {
      error (0, 0, _("\
format string may not be specified when printing equal width strings"));
      usage (1);
    }

  if (!step_is_set)
    {
      step = first <= last ? 1.0 : -1.0;
    }

  if (format_str != NULL)
    {
      if (!check_format (format_str))
	{
	  error (0, 0, _("invalid format string: `%s'"), format_str);
	  usage (1);
	}
    }
  else
    {
      if (equal_width)
	format_str = get_width_format ();
      else
	format_str = "%g";
    }

  errs = print_numbers (format_str);

  exit (errs);
  /* NOTREACHED */
}

/* Read a double value from the command line.
   Return if the string is correct else signal error.  */

static double
scan_double_arg (const char *arg)
{
  double ret_val;

  if (xstrtod (arg, NULL, &ret_val))
    {
      error (0, 0, _("invalid floating point argument: %s"), arg);
      usage (1);
      /* NOTREACHED */
    }

  return ret_val;
}

/* Check whether the format string is valid for a single double
   argument.
   Return 0 if not, 1 if correct.  */

static int
check_format (const char *fmt)
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
    return 0;

  fmt += strspn (fmt, "-+#0");
  if (ISDIGIT (*fmt))
    {
      fmt += strspn (fmt, "0123456789");

      if (*fmt == '.')
	fmt += strspn (++fmt, "0123456789");
    }

  if (*fmt != 'e' && *fmt != 'f' && *fmt != 'g')
    return 0;

  fmt++;
  while (*fmt != '\0')
    {
      if (*fmt == '%')
	{
	  fmt++;
	  if (*fmt != '%')
	    return 0;
	}

      fmt++;
    }

  return 1;
}

#if defined (HAVE_RINT) && defined (HAVE_MODF) && defined (HAVE_FLOOR)

/* Return a printf-style format string with which all selected numbers
   will format to strings of the same width.  */

static char *
get_width_format ()
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
  if (buffer[strspn (buffer, "0123456789")] != '\0')
    return "%g";
  width1 = strlen (buffer);

  if (min_val < 0.0)
    {
      sprintf (buffer, "%g", rint (min_val));
      if (buffer[strspn (buffer, "-0123456789")] != '\0')
	return "%g";
      width2 = strlen (buffer);

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
	  /* FIXME: assumes that decimal_point is a single character
	     string.  */
	  || buffer[1] != decimal_point[0]
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
	  /* FIXME: assumes that decimal_point is a single character
	     string.  */
	  || buffer[1] != decimal_point[0]
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

/* Actually print the sequence of numbers in the specified range, with the
   given or default stepping and format.  */
static int
print_numbers (const char *fmt)
{
  if (first > last)
    {
      int i;

      if (step >= 0)
	{
	  error (0, 0,
		 _("when the starting value is larger than the limit,\n\
the increment must be negative"));
	  usage (1);
	  /* NOTREACHED */
	}

      printf (fmt, first);
      for (i = 1; /* empty */; i++)
	{
	  double x = first + i * step;

	  if (x < last)
	    break;

	  fputs (separator, stdout);
	  printf (fmt, x);
	}
    }
  else
    {
      int i;

      if (step <= 0)
	{
	  error (0, 0,
		 _("when the starting value is smaller than the limit,\n\
the increment must be positive"));
	  usage (1);
	  /* NOTREACHED */
	}

      printf (fmt, first);
      for (i = 1; /* empty */; i++)
	{
	  double x = first + i * step;

	  if (x > last)
	    break;

	  fputs (separator, stdout);
	  printf (format_str, x);
	}
    }
  fputs (terminator, stdout);

  return 0;
}
