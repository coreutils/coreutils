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

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"

/* This is portable and avoids bringing in all of the ctype stuff. */
#define isdigit(c) ((c) >= '0' && (c) <= '9')

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
void show_date ();
void usage ();

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

  while ((optc = getopt (argc, argv, "d:s:u")) != EOF)
    switch (optc)
      {
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
	usage ();
      }

  if (argc - optind > 1)
    usage ();

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

void
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

void
usage ()
{
  fprintf (stderr, "\
Usage: %s [-u] [-d datestr] [-s datestr] [+FORMAT] [MMDDhhmm[[CC]YY][.ss]]\n",
	   program_name);
  exit (1);
}
