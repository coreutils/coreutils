/* seq - print sequence of numbers to standard output.
   Copyright (C) 94, 1995 Free Software Foundation, Inc.

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

/* Written by Ulrich Drepper.  */

#include <config.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>

#include "system.h"
#include "error.h"

static double scan_double_arg __P ((char *arg));
static int check_format __P ((char *format_string));
static char *get_width_format __P ((void));
static int print_numbers __P ((char *format_str));

/* If nonzero print all number with equal width.  */
static int equal_width;

/* The printf(3) format used for output.  */
static char *format_str;

/* The starting number.  */
static double from;

/* The name that this program was run with.  */
char *program_name;

/* The string used to separate two number.  */
static char *separator;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

/* The increment.  */
static double step;

/* The last number.  */
static double last;

static struct option const long_options[] =
{
  { "equal-width", no_argument, NULL, 'w'},
  { "format", required_argument, NULL, 'f'},
  { "help", no_argument, &show_help, 1},
  { "separator", required_argument, NULL, 's'},
  { "version", no_argument, &show_version, 1},
  { NULL, 0, NULL, 0}
};

static void
usage (int status)
{
  if (status != 0)
    (void) fprintf (stderr, _("Try `%s --help' for more information.\n"),
		    program_name);
  else
    {
      (void) printf (_("\
Usage: %s [OPTION]... [from [step]] to\n\
"), program_name);
      (void) printf (_("\
\n\
  -f, --format FORMAT      use printf(3) style FORMAT (default: %%g)\n\
      --help               display this help and exit\n\
  -s, --separator STRING   use STRING for separating numbers (default: \\n)\n\
      --version            output version information and exit\n\
  -w, --equal-width        equalize width by padding with leading zeroes\n\
\n\
  FROM, STEP, TO are interpreted as floating point.  STEP should be > 0 if\n\
  FROM is smaller than TO and vice versa.  When given, the FORMAT argument\n\
  must contain exactly one of the float output formats %%e, %%f, or %%g.\n\
"));
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
  from = 1.0;
  step_is_set = 0;

  /* We have to handle negative numbers in the command line but this
     conflicts with the command line arguments.  So the getopt mode is
     REQUIRE_ORDER (the '+' in the format string) and it abort on the
     first non-option or negative number.  */
  while ((optc = getopt_long (argc, argv, "+0123456789f:s:w", long_options,
			      (int *) 0)) != EOF)
    {
      if ('0' <= optc && optc <= '9')
	{
	  /* means negative number */
	  break;
	}

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

	default:
	  usage (1);
	  /* NOTREACHED */
	}
    }

  if (show_version)
    {
      (void) printf ("seq - %s\n", PACKAGE_VERSION);
      exit (0);
    }

  if (show_help)
    {
      usage (0);
      /* NOTREACHED */
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
      from = last;
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
      step = from <= last ? 1.0 : -1.0;
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
scan_double_arg (char *arg)
{
  char *end_ptr;
  double ret_val;

  /* FIXME: use xstrtod?  At least set and check errno.  */
  ret_val = strtod (arg, &end_ptr);
  if (end_ptr == arg || *end_ptr != '\0')
    {
      error (0, 0, _("invalid float argument: %s"), arg);
      usage (1);
      /* NOTREACHED */
    }

  return ret_val;
}

/* Check whether the format string is valid for a single double
   argument.
   Return 0 if not, 1 if correct.  */

static int
check_format (char *format_string)
{
  while (*format_string != '\0')
    {
      if (*format_string == '%')
	{
	  format_string++;
	  if (*format_string != '%')
	    break;
	}

      format_string++;
    }
  if (*format_string == '\0')
    return 0;

  format_string += strspn (format_string, "-+#0");
  if (isdigit (*format_string))
    {
      format_string += strspn (format_string, "012345789");

      if (*format_string == '.')
	format_string += strspn (++format_string, "0123456789");
    }

  if (*format_string != 'e' && *format_string != 'f' &&
      *format_string != 'g')
    return 0;

  format_string++;
  while (*format_string != '\0')
    {
      if (*format_string == '%')
	{
	  format_string++;
	  if (*format_string != '%')
	    return 0;
	}

      format_string++;
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

  if (from > last)
    {
      min_val = from - step * floor ((from - last) / step);
      max_val = from;
    }
  else
    {
      min_val = from;
      max_val = from + step * floor ((last - from) / step);
    }

  (void) sprintf (buffer, "%g", rint (max_val));
  if (buffer[strspn (buffer, "0123456789")] != '\0')
    return "%g";
  width1 = strlen (buffer);

  if (min_val < 0.0)
    {
      (void) sprintf (buffer, "%g", rint (min_val));
      if (buffer[strspn (buffer, "-0123456789")] != '\0')
	return "%g";
      width2 = strlen (buffer);

      width1 = width1 > width2 ? width1 : width2;
    }
  full_width = width1;

  (void) sprintf (buffer, "%g", 1.0 + modf (min_val, &temp));
  width1 = strlen (buffer);
  if (width1 == 1)
    width1 = 0;
  else
    {
      if (buffer[0] != '1' || buffer[1] != '.' ||
	  buffer[2 + strspn (&buffer[2], "0123456789")] != '\0')
	return "%g";
      width1 -= 2;
    }

  (void) sprintf (buffer, "%g", 1.0 + modf (step, &temp));
  width2 = strlen (buffer);
  if (width2 == 1)
    width2 = 0;
  else
    {
      if (buffer[0] != '1' || buffer[1] != '.' ||
	  buffer[2 + strspn (&buffer[2], "0123456789")] != '\0')
	return "%g";
      width2 -= 2;
    }
  frac_width = width1 > width2 ? width1 : width2;

  if (frac_width)
    (void) sprintf (buffer, "%%0%d.%df", full_width + 1 + frac_width, frac_width);
  else
    (void) sprintf (buffer, "%%0%dg", full_width);

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
print_numbers (char *format_str)
{
  if (from > last)
    {
      if (step >= 0)
	{
	  error (0, 0, _("invalid increment: %g"), step);
	  usage (1);
	  /* NOTREACHED */
	}

      while (1)
	{
	  (void) printf (format_str, from);

	  /* FIXME: don't increment!!!  Use `first + i * step'.  */
	  from += step;
	  if (from < last)
	    break;

	  (void) fputs (separator, stdout);
	}
    }
  else
    {
      if (step <= 0)
	{
	  error (0, 0, _("invalid increment: %g"), step);
	  usage (1);
	  /* NOTREACHED */
	}

      while (1)
	{
	  (void) printf (format_str, from);

	  /* FIXME: don't increment!!!  Use `first + i * step'.  */
	  from += step;
	  if (from > last)
	    break;

	  (void) fputs (separator, stdout);
	}
    }

  return 0;
}
