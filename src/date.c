/* date - print or set the system date and time
   Copyright (C) 1989, 1991 Free Software Foundation, Inc.

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

/* Options:
   -d DATESTR	Display the date DATESTR.
   -s DATESTR	Set the date to DATESTR.
   -u		Display or set the date in universal instead of local time.
   +FORMAT	Specify custom date output format, described below.
   MMDDhhmm[[CC]YY][.ss]	Set the date in the format described below.

   If one non-option argument is given, it is used as the date to which
   to set the system clock, and must have the format:
   MM	month (01..12)
   DD	day in month (01..31)
   hh	hour (00..23)
   mm	minute (00..59)
   CC	first 2 digits of year (optional, defaults to current) (00..99)
   YY	last 2 digits of year (optional, defaults to current) (00..99)
   ss	second (00..61)

   If a non-option argument that starts with a `+' is specified, it
   is used to control the format in which the date is printed; it
   can contain any of the `%' substitutions allowed by the strftime
   function.  A newline is always added at the end of the output.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "version.h"
#include "system.h"

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifndef STDC_HEADERS
time_t mktime ();
size_t strftime ();
time_t time ();
#endif

int putenv ();
int stime ();

char *xrealloc ();
time_t get_date ();
time_t posixtime ();
void error ();

static void show_date ();
static void usage ();

/* putenv string to use Universal Coordinated Time.
   POSIX.2 says it should be "TZ=UCT0" or "TZ=GMT0". */
#ifndef TZ_UCT
#if defined(hpux) || defined(__hpux__) || defined(ultrix) || defined(__ultrix__) || defined(USG)
#define TZ_UCT "TZ=GMT0"
#else
#define TZ_UCT "TZ="
#endif
#endif

/* The name this program was run with, for error messages. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"date", required_argument, NULL, 'd'},
  {"help", no_argument, &show_help, 1},
  {"set", required_argument, NULL, 's'},
  {"uct", no_argument, NULL, 'u'},
  {"universal", no_argument, NULL, 'u'},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;
  char *datestr = NULL;
  time_t when;
  int set_date = 0;
  int universal_time = 0;

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "d:s:u", long_options, (int *) 0))
	 != EOF)
    switch (optc)
      {
      case 0:
	break;
      case 'd':
	datestr = optarg;
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
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (argc - optind > 1)
    usage (1);

  if (universal_time && putenv (TZ_UCT) != 0)
    error (1, 0, "virtual memory exhausted");

  time (&when);

  if (datestr)
    when = get_date (datestr, NULL);

  if (argc - optind == 1 && argv[optind][0] != '+')
    {
      when = posixtime (argv[optind]);
      set_date = 1;
    }

  if (when == -1)
    error (1, 0, "invalid date");

  if (set_date && stime (&when) == -1)
    error (0, errno, "cannot set date");

  if (argc - optind == 1 && argv[optind][0] == '+')
    show_date (argv[optind] + 1, when);
  else
    show_date ((char *) NULL, when);

  exit (0);
}

/* Display the date and/or time in WHEN according to the format specified
   in FORMAT, followed by a newline.  If FORMAT is NULL, use the
   standard output format (ctime style but with a timezone inserted). */

static void
show_date (format, when)
     char *format;
     time_t when;
{
  struct tm *tm;
  char *out = NULL;
  size_t out_length = 0;

  tm = localtime (&when);

  if (format == NULL)
    /* Print the date in the default format.  Vanilla ANSI C strftime
       doesn't support %e, but POSIX requires it.  If you don't use
       a GNU strftime, make sure yours supports %e.  */
    format = "%a %b %e %H:%M:%S %Z %Y";
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
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [+FORMAT] [MMDDhhmm[[CC]YY][.ss]]\n",
	      program_name);
      printf ("\
\n\
  -d, --date=STRING        display time described by STRING, not `now'\n\
  -s, --set=STRING         set time described by STRING\n\
  -u, --uct, --universal   print or set Universal Coordinated Time\n\
      --help               display this help and exit\n\
      --version            output version information and exit\n\
");
      printf ("\
\n\
FORMAT controls the output.  Interpreted sequences are:\n\
\n\
  %%%%   a literal %%\n\
  %%A   locale's full weekday name, variable length (Sunday..Saturday)\n\
  %%B   locale's full month name, variable length (January..December)\n\
  %%D   date (mm/dd/yy)\n\
  %%H   hour (00..23)\n\
  %%I   hour (01..12)\n\
  %%M   minute (00..59)\n\
  %%S   second (00..61)\n\
  %%T   time, 24-hour (hh:mm:ss)\n\
  %%U   week number of year with Sunday as first day of week (00..53)\n\
  %%W   week number of year with Monday as first day of week (00..53)\n\
  %%X   locale's time representation (%%H:%%M:%%S)\n\
  %%Y   year (1970...)\n\
  %%Z   time zone (e.g., EDT), or nothing if no time zone is determinable\n\
  %%a   locale's abbreviated weekday name (Sun..Sat)\n\
  %%b   locale's abbreviated month name (Jan..Dec)\n\
  %%c   locale's date and time (Sat Nov 04 12:02:33 EST 1989)\n\
  %%d   day of month (01..31)\n\
  %%h   same as %%b\n\
  %%j   day of year (001..366)\n\
  %%k   hour ( 0..23)\n\
  %%l   hour ( 1..12)\n\
  %%m   month (01..12)\n\
  %%n   a newline\n\
  %%p   locale's AM or PM\n\
  %%r   time, 12-hour (hh:mm:ss [AP]M)\n\
  %%t   a horizontal tab\n\
  %%w   day of week (0..6)\n\
  %%x   locale's date representation (mm/dd/yy)\n\
  %%y   last two digits of year (00..99)\n\
");
    }
  exit (status);
}
