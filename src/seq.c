/* seq - print sequence of numbers to standard output.
   Copyright (C) 1994 Free Software Foundation, Inc.

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

/* Ulrich Drepper */

#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"

static double scan_double_arg ();
static int check_format ();
static char *get_width_format ();
static int print_numbers ();

/* If non-zero print all number with equal width. */
static int equal_width;

/* The printf(3) format used for output. */
static char *format_str;

/* The starting number. */
static double from;

/* The name that this program was run with. */
char *program_name;

/* The string used to seperate two number. */
static char *seperator;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

/* The increment. */
static double step;

/* The last number. */
static double last;

static struct option const long_options[] =
{
  { "equal-width", no_argument, NULL, 'w'},
  { "format", required_argument, NULL, 'f'},
  { "help", no_argument, &show_help, 1},
  { "seperator", required_argument, NULL, 's'},
  { "version", no_argument, &show_version, 1},
  { NULL, 0, NULL, 0}
};

static void
usage (status)
     int status;
{
  if (status != 0)
    (void) fprintf (stderr, "Try `%s --help' for more information.\n",
		    program_name);
  else
    {
      (void) printf ("\
Usage: %s [OPTION]... [from [step]] to\n\
", program_name);
      (void) printf ("\
\n\
  -f, --format FORMAT      use printf(3) style FORMAT (default: %%g)\n\
      --help               display this help and exit\n\
  -s, --seperator STRING   use STRING for seperating numbers (default: \\n)\n\
      --version            output version information and exit\n\
  -w, --equal-width        equalize width by padding with leading zeroes\n\
\n\
  FROM, STEP, TO are interpreted as floating point.  STEP should be > 0 if\n\
  FROM is smaller than TO and vice versa.  When given, the FORMAT argument\n\
  must contain exactly one of the float output formats %%e, %%f, or %%g.\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  int errs;
  int optc;
  int step_is_set;

  program_name = argv[0];
  equal_width = 0;
  format_str = NULL;
  seperator = "\n";
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
	  seperator = optarg;
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
      (void) printf ("seq - %s\n", version_string);
    }

  if (show_help)
    {
      usage (0);
      /* NOTREACHED */
    }

  if (optind >= argc)
    {
      usage (2);
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
	      usage (2);
	      /* NOTREACHED */
	    }
	}
    }

  if (!step_is_set)
    {
      step = from < last ? 1.0 : -1.0;
    }

  if (format_str != NULL)
    {
      if (!check_format (format_str))
	{
	  fprintf (stderr, "illegal format string\n");
	  usage (4);
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

/* Read an double value from the command line.
   Return if the string is correct else signal error.  */

static double
scan_double_arg (arg)
     char *arg;
{
  char *end_ptr;
  double ret_val;

  ret_val = strtod (arg, &end_ptr);
  if (end_ptr == arg || *end_ptr != '\0')
    {
      fprintf (stderr, "illegal float argument: %s\n", arg);
      usage (2);
      /* NOTREACHED */
    }

  return ret_val;
}

/* Check whether the format string is valid for a single double
   argument.
   Return 0 if not, 1 if correct.  */

static int
check_format (format_string)
     char *format_string;
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

/* Returns the format for that all printed numbers have the same width.  */
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

/* Actually print the sequence of numbers in the specified range, with the
   given or default stepping and format.  */
static int
print_numbers (format_str)
     char *format_str;
{
  if (from > last)
    {
      if (step >= 0)
	{
	  (void) fprintf (stderr, "illegal increment: %g\n", step);
	  usage (2);
	  /* NOTREACHED */
	}

      while (1)
	{
	  (void) printf (format_str, from);

	  from += step;
	  if (from < last)
	    break;

	  (void) fputs (seperator, stdout);
	}
    }
  else
    {
      if (step <= 0)
	{
	  (void) fprintf (stderr, "illegal increment: %g\n", step);
	  usage (2);
	  /* NOTREACHED */
	}

      while (1)
	{
	  (void) printf (format_str, from);

	  from += step;
	  if (from > last)
	    break;

	  (void) fputs (seperator, stdout);
	}
    }

  return 0;
}
