/* date - print or set the system date and time
   Copyright (C) 89, 90, 91, 92, 93, 94, 1995 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "version.h"
#include "system.h"
#include "getline.h"
#include "error.h"

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef STDC_HEADERS
size_t strftime ();
time_t time ();
#endif

int stime ();

char *xrealloc ();
time_t get_date ();
time_t posixtime ();

static void show_date ();
static void usage ();

/* The name this program was run with, for error messages. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

/* If non-zero, print or set Coordinated Universal Time.  */
static int universal_time = 0;

static struct option const long_options[] =
{
  {"date", required_argument, NULL, 'd'},
  {"file", required_argument, NULL, 'f'},
  {"help", no_argument, &show_help, 1},
  {"set", required_argument, NULL, 's'},
  {"uct", no_argument, NULL, 'u'},
  {"utc", no_argument, NULL, 'u'},
  {"universal", no_argument, NULL, 'u'},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

/* Parse each line in INPUT_FILENAME as with --date and display the
   each resulting time and date.  If the file cannot be opened, tell why
   then exit.  Issue a diagnostic for any lines that cannot be parsed.
   If any line cannot be parsed, return non-zero;  otherwise return zero.  */

static int
batch_convert (input_filename, format)
     const char *input_filename;
     const char *format;
{
  int have_read_stdin;
  int status;
  FILE *in_stream;
  char *line;
  int line_length;
  int buflen;
  time_t when;

  if (strcmp (input_filename, "-") == 0)
    {
      input_filename = _("standard input");
      in_stream = stdin;
      have_read_stdin = 1;
    }
  else
    {
      in_stream = fopen (input_filename, "r");
      if (in_stream == NULL)
	{
	  error (0, errno, "%s", input_filename);
	}
      have_read_stdin = 0;
    }

  line = NULL;
  buflen = 0;

  status = 0;
  while (1)
    {
      line_length = getline (&line, &buflen, in_stream);
      if (line_length < 0)
	{
	  /* FIXME: detect/handle error here.  */
	  break;
	}
      when = get_date (line, NULL);
      if (when == -1)
	{
	  if (line[line_length - 1] == '\n')
	    line[line_length - 1] = '\0';
	  error (0, 0, _("invalid date `%s'"), line);
	  status = 1;
	}
      else
	{
	  show_date (format, when);
	}
    }

  if (have_read_stdin && fclose (stdin) == EOF)
    error (2, errno, _("standard input"));

  if (line != NULL)
    free (line);

  return status;
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;
  const char *datestr = NULL;
  time_t when;
  int set_date = 0;
  int print_date = 0;
  char *format;
  char *batch_file = NULL;
  int n_args;
  int status;

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "d:f:s:u", long_options, (int *) 0))
	 != EOF)
    switch (optc)
      {
      case 0:
	break;
      case 'd':
	datestr = optarg;
	print_date = 1;
	break;
      case 'f':
	batch_file = optarg;
	break;
      case 's':
	datestr = optarg;
	set_date = 1;
	break;
      case 'u':
	universal_time = 1;
	break;
      default:
	usage (1);
      }

  if (show_version)
    {
      printf ("date - %s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  n_args = argc - optind;

  if (set_date && print_date)
    {
      error (0, 0,
	  _("the options to print and set the time may not be used together"));
      usage (1);
    }

  if (n_args > 1)
    {
      error (0, 0, _("too many non-option arguments"));
      usage (1);
    }

  if ((set_date || print_date || batch_file != NULL)
      && n_args == 1 && argv[optind][0] != '+')
    {
      error (0, 0, _("\
when using the print, set time, or batch options, any\n\
non-option argument must be a format string beginning with `+'"));
      usage (1);
    }

  if (batch_file != NULL)
    {
      if (set_date || print_date)
	{
	  error (0, 0, _("\
neither print nor set options may be used when reading dates from a file"));
	  usage (1);
	}
      status = batch_convert (batch_file,
			      (n_args == 1 ? argv[optind] + 1 : NULL));
    }
  else
    {
      status = 0;

      if (!print_date && !set_date)
	{
	  if (n_args == 1 && argv[optind][0] != '+')
	    {
	      /* Prepare to set system clock to the specified date/time
		 given in the POSIX-format.  */
	      set_date = 1;
	      datestr = argv[optind];
	      when = posixtime (datestr);
	      format = NULL;
	    }
	  else
	    {
	      /* Prepare to print the current date/time.  */
	      print_date = 1;
	      datestr = _("undefined");
	      time (&when);
	      format = (n_args == 1 ? argv[optind] + 1 : NULL);
	    }
	}
      else
	{
	  /* (print_date || set_date) */
	  when = get_date (datestr, NULL);
	  format = (n_args == 1 ? argv[optind] + 1 : NULL);
	}

      if (when == -1)
	error (1, 0, _("invalid date `%s'"), datestr);

      if (set_date)
	{
	  /* Set the system clock to the specified date, then regardless of
	     the success of that operation, format and print that date.  */
	  if (stime (&when) == -1)
	    error (0, errno, _("cannot set date"));
	}

      show_date (format, when);
    }

  if (fclose (stdout) == EOF)
    error (2, errno, _("write error"));

  exit (status);
}

/* Display the date and/or time in WHEN according to the format specified
   in FORMAT, followed by a newline.  If FORMAT is NULL, use the
   standard output format (ctime style but with a timezone inserted). */

static void
show_date (format, when)
     const char *format;
     time_t when;
{
  struct tm *tm;
  char *out = NULL;
  size_t out_length = 0;

  tm = (universal_time ? gmtime : localtime) (&when);

  if (format == NULL)
    {
      /* Print the date in the default format.  Vanilla ANSI C strftime
         doesn't support %e, but POSIX requires it.  If you don't use
         a GNU strftime, make sure yours supports %e.  */
      format = (universal_time
		? "%a %b %e %H:%M:%S UTC %Y"
		: "%a %b %e %H:%M:%S %Z %Y");
    }
  else if (*format == '\0')
    {
      printf ("\n");
      return;
    }

  do
    {
      out_length += 200;
      out = (char *) xrealloc (out, out_length);
    }
  while (strftime (out, out_length, format, tm) == 0);

  printf ("%s\n", out);
  free (out);
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [+FORMAT]\n\
  or:  %s [OPTION] [MMDDhhmm[[CC]YY][.ss]]\n\
"),
	      program_name, program_name);
      printf (_("\
Display the current time in the given FORMAT, or set the system date.\n\
\n\
  -d, --date=STRING        display time described by STRING, not `now'\n\
  -f, --file=DATEFILE      like --date once for each line of DATEFILE\n\
  -s, --set=STRING         set time described by STRING\n\
  -u, --utc, --universal   print or set Coordinated Universal Time\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
"));
      printf (_("\
\n\
FORMAT controls the output.  The only valid option for the second form\n\
specifies Coordinated Universal Time.  Interpreted sequences are:\n\
\n\
  %%%%   a literal %%\n\
  %%a   locale's abbreviated weekday name (Sun..Sat)\n\
  %%A   locale's full weekday name, variable length (Sunday..Saturday)\n\
  %%b   locale's abbreviated month name (Jan..Dec)\n\
  %%B   locale's full month name, variable length (January..December)\n\
  %%c   locale's date and time (Sat Nov 04 12:02:33 EST 1989)\n\
  %%d   day of month (01..31)\n\
  %%D   date (mm/dd/yy)\n\
  %%h   same as %%b\n\
  %%H   hour (00..23)\n\
  %%I   hour (01..12)\n\
  %%j   day of year (001..366)\n\
  %%k   hour ( 0..23)\n\
  %%l   hour ( 1..12)\n\
  %%m   month (01..12)\n\
  %%M   minute (00..59)\n\
  %%n   a newline\n\
  %%p   locale's AM or PM\n\
  %%r   time, 12-hour (hh:mm:ss [AP]M)\n\
  %%s   seconds since 00:00:00, Jan 1, 1970 (a GNU extension)\n\
  %%S   second (00..61)\n\
  %%t   a horizontal tab\n\
  %%T   time, 24-hour (hh:mm:ss)\n\
  %%U   week number of year with Sunday as first day of week (00..53)\n\
  %%w   day of week (0..6);  0 represents Sunday\n\
  %%W   week number of year with Monday as first day of week (00..53)\n\
  %%x   locale's date representation (mm/dd/yy)\n\
  %%X   locale's time representation (%%H:%%M:%%S)\n\
  %%y   last two digits of year (00..99)\n\
  %%Y   year (1970...)\n\
  %%Z   time zone (e.g., EDT), or nothing if no time zone is determinable\n\
\n\
By default, date pads numeric fields with zeroes.  GNU date recognizes\n\
the following modifiers between `%%' and a numeric directive.\n\
\n\
  `-' (hyphen) do not pad the field\n\
  `_' (underscore) pad the field with spaces\n\
"));
    }
  exit (status);
}
