/* date - print or set the system date and time
   Copyright (C) 1989-2001 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#if HAVE_LANGINFO_H
# include <langinfo.h>
#endif

#include "system.h"
#include "argmatch.h"
#include "closeout.h"
#include "error.h"
#include "getdate.h"
#include "getline.h"
#include "posixtm.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "date"

#define AUTHORS "David MacKenzie"

#ifndef STDC_HEADERS
size_t strftime ();
time_t time ();
#endif

int putenv ();
int stime ();

static void show_date PARAMS ((const char *format, time_t when));

enum Time_spec
{
  /* display only the date: 1999-03-25 */
  TIME_SPEC_DATE=1,
  /* display date and hour: 1999-03-25T03-0500 */
  TIME_SPEC_HOURS,
  /* display date, hours, and minutes: 1999-03-25T03:23-0500 */
  TIME_SPEC_MINUTES,
  /* display date, hours, minutes, and seconds: 1999-03-25T03:23:14-0500 */
  TIME_SPEC_SECONDS
};

static char const *const time_spec_string[] =
{
  "date", "hours", "minutes", "seconds", 0
};

static enum Time_spec const time_spec[] =
{
  TIME_SPEC_DATE, TIME_SPEC_HOURS, TIME_SPEC_MINUTES, TIME_SPEC_SECONDS
};

/* The name this program was run with, for error messages. */
char *program_name;

/* If nonzero, display an ISO 8601 format date/time string */
static int iso_8601_format = 0;

/* If non-zero, display time in RFC-(2)822 format for mail or news. */
static int rfc_format = 0;

static struct option const long_options[] =
{
  {"date", required_argument, NULL, 'd'},
  {"file", required_argument, NULL, 'f'},
  {"iso-8601", optional_argument, NULL, 'I'},
  {"reference", required_argument, NULL, 'r'},
  {"rfc-822", no_argument, NULL, 'R'},
  {"set", required_argument, NULL, 's'},
  {"uct", no_argument, NULL, 'u'},
  {"utc", no_argument, NULL, 'u'},
  {"universal", no_argument, NULL, 'u'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

#if LOCALTIME_CACHE
# define TZSET tzset ()
#else
# define TZSET /* empty */
#endif

#ifdef _DATE_FMT
# define DATE_FMT_LANGINFO() nl_langinfo (_DATE_FMT)
#else
# define DATE_FMT_LANGINFO() ""
#endif

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [+FORMAT]\n\
  or:  %s [-u|--utc|--universal] [MMDDhhmm[[CC]YY][.ss]]\n\
"),
	      program_name, program_name);
      printf (_("\
Display the current time in the given FORMAT, or set the system date.\n\
\n\
  -d, --date=STRING         display time described by STRING, not `now'\n\
  -f, --file=DATEFILE       like --date once for each line of DATEFILE\n\
  -I, --iso-8601[=TIMESPEC] output an ISO-8601 compliant date/time string.\n\
                            TIMESPEC=`date' (or missing) for date only,\n\
                            `hours', `minutes', or `seconds' for date and\n\
                            time to the indicated precision.\n\
  -r, --reference=FILE      display the last modification time of FILE\n\
  -R, --rfc-822             output RFC-822 compliant date string\n\
  -s, --set=STRING          set time described by STRING\n\
  -u, --utc, --universal    print or set Coordinated Universal Time\n\
      --help                display this help and exit\n\
      --version             output version information and exit\n\
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
  %%C   century (year divided by 100 and truncated to an integer) [00-99]\n\
  %%d   day of month (01..31)\n\
  %%D   date (mm/dd/yy)\n\
  %%e   day of month, blank padded ( 1..31)\n\
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
  %%s   seconds since `00:00:00 1970-01-01 UTC' (a GNU extension)\n\
  %%S   second (00..60)\n\
  %%t   a horizontal tab\n\
  %%T   time, 24-hour (hh:mm:ss)\n\
  %%U   week number of year with Sunday as first day of week (00..53)\n\
  %%V   week number of year with Monday as first day of week (01..53)\n\
  %%w   day of week (0..6);  0 represents Sunday\n\
  %%W   week number of year with Monday as first day of week (00..53)\n\
  %%x   locale's date representation (mm/dd/yy)\n\
  %%X   locale's time representation (%%H:%%M:%%S)\n\
  %%y   last two digits of year (00..99)\n\
  %%Y   year (1970...)\n\
  %%z   RFC-822 style numeric timezone (-0500) (a nonstandard extension)\n\
  %%Z   time zone (e.g., EDT), or nothing if no time zone is determinable\n\
\n\
By default, date pads numeric fields with zeroes.  GNU date recognizes\n\
the following modifiers between `%%' and a numeric directive.\n\
\n\
  `-' (hyphen) do not pad the field\n\
  `_' (underscore) pad the field with spaces\n\
"));
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

/* Parse each line in INPUT_FILENAME as with --date and display each
   resulting time and date.  If the file cannot be opened, tell why
   then exit.  Issue a diagnostic for any lines that cannot be parsed.
   If any line cannot be parsed, return nonzero;  otherwise return zero.  */

static int
batch_convert (const char *input_filename, const char *format)
{
  int status;
  FILE *in_stream;
  char *line;
  int line_length;
  size_t buflen;
  time_t when;

  if (strcmp (input_filename, "-") == 0)
    {
      input_filename = _("standard input");
      in_stream = stdin;
    }
  else
    {
      in_stream = fopen (input_filename, "r");
      if (in_stream == NULL)
	{
	  error (1, errno, "`%s'", input_filename);
	}
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

  if (fclose (in_stream) == EOF)
    error (2, errno, "`%s'", input_filename);

  if (line != NULL)
    free (line);

  return status;
}

int
main (int argc, char **argv)
{
  int optc;
  const char *datestr = NULL;
  const char *set_datestr = NULL;
  time_t when;
  int set_date = 0;
  char *format;
  char *batch_file = NULL;
  char *reference = NULL;
  struct stat refstats;
  int n_args;
  int status;
  int option_specified_date;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  close_stdout_set_status (2);
  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "d:f:I::r:Rs:u", long_options, NULL))
	 != -1)
    switch (optc)
      {
      case 0:
	break;
      case 'd':
	datestr = optarg;
	break;
      case 'f':
	batch_file = optarg;
	break;
      case 'I':
	iso_8601_format = (optarg
			   ? XARGMATCH ("--iso-8601", optarg,
					time_spec_string, time_spec)
			   : TIME_SPEC_DATE);
	break;
      case 'r':
	reference = optarg;
	break;
      case 'R':
	rfc_format = 1;
	break;
      case 's':
	set_datestr = optarg;
	set_date = 1;
	break;
      case 'u':
	/* POSIX.2 says that `date -u' is equivalent to setting the TZ
	   environment variable, so this option should do nothing other
	   than setting TZ.  */
	if (putenv ("TZ=UTC0") != 0)
	  xalloc_die ();
	TZSET;
	break;
      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
	usage (1);
      }

  n_args = argc - optind;

  option_specified_date = ((datestr ? 1 : 0)
			   + (batch_file ? 1 : 0)
			   + (reference ? 1 : 0));

  if (option_specified_date > 1)
    {
      error (0, 0,
	_("the options to specify dates for printing are mutually exclusive"));
      usage (1);
    }

  if (set_date && option_specified_date)
    {
      error (0, 0,
	  _("the options to print and set the time may not be used together"));
      usage (1);
    }

  if (n_args > 1)
    {
      error (0, 0, _("too many non-option arguments: %s%s"),
	     argv[optind + 1], n_args == 2 ? "" : " ...");
      usage (1);
    }

  if ((set_date || option_specified_date)
      && n_args == 1 && argv[optind][0] != '+')
    {
      error (0, 0, _("\
the argument `%s' lacks a leading `+';\n\
When using an option to specify date(s), any non-option\n\
argument must be a format string beginning with `+'."),
	     argv[optind]);
      usage (1);
    }

  /* Simply ignore --rfc-822 if specified when setting the date.  */
  if (rfc_format && !set_date && n_args > 0)
    {
      error (0, 0,
	     _("a format string may not be specified when using\
 the --rfc-822 (-R) option"));
      usage (1);
    }

  if (set_date)
    datestr = set_datestr;

  if (batch_file != NULL)
    {
      status = batch_convert (batch_file,
			      (n_args == 1 ? argv[optind] + 1 : NULL));
    }
  else
    {
      status = 0;

      if (!option_specified_date && !set_date)
	{
	  if (n_args == 1 && argv[optind][0] != '+')
	    {
	      /* Prepare to set system clock to the specified date/time
		 given in the POSIX-format.  */
	      set_date = 1;
	      datestr = argv[optind];
	      when = posixtime (datestr,
				PDS_TRAILING_YEAR | PDS_CENTURY | PDS_SECONDS);
	      format = NULL;
	    }
	  else
	    {
	      /* Prepare to print the current date/time.  */
	      datestr = _("undefined");
	      time (&when);
	      format = (n_args == 1 ? argv[optind] + 1 : NULL);
	    }
	}
      else
	{
	  /* (option_specified_date || set_date) */
	  if (reference != NULL)
	    {
	      if (stat (reference, &refstats))
		error (1, errno, "%s", reference);
	      when = refstats.st_mtime;
	    }
	  else
	    {
	      when = get_date (datestr, NULL);
	    }

	  format = (n_args == 1 ? argv[optind] + 1 : NULL);
	}

      if (when == -1)
	error (1, 0, _("invalid date `%s'"), datestr);

      if (set_date)
	{
	  /* Set the system clock to the specified date, then regardless of
	     the success of that operation, format and print that date.  */
	  if (stime (&when) == -1)
	    {
	      error (0, errno, _("cannot set date"));
	      status = 1;
	    }
	}

      show_date (format, when);
    }

  exit (status);
}

/* Display the date and/or time in WHEN according to the format specified
   in FORMAT, followed by a newline.  If FORMAT is NULL, use the
   standard output format (ctime style but with a timezone inserted). */

static void
show_date (const char *format, time_t when)
{
  struct tm *tm;
  char *out = NULL;
  size_t out_length = 0;
  /* ISO 8601 formats.  See below regarding %z */
  static char const * const iso_format_string[] =
  {
    "%Y-%m-%d",
    "%Y-%m-%dT%H%z",
    "%Y-%m-%dT%H:%M%z",
    "%Y-%m-%dT%H:%M:%S%z"
  };

  tm = localtime (&when);

  if (format == NULL)
    {
      /* Print the date in the default format.  Vanilla ANSI C strftime
         doesn't support %e, but POSIX requires it.  If you don't use
         a GNU strftime, make sure yours supports %e.
	 If you are not using GNU strftime, you want to change %z
	 in the RFC format to %Z; this gives, however, an invalid
	 RFC time format outside the continental United States and GMT. */

      if (rfc_format)
	format = "%a, %d %b %Y %H:%M:%S %z";
      else if (iso_8601_format)
	format = iso_format_string[iso_8601_format - 1];
      else
	{
	  char *date_fmt = DATE_FMT_LANGINFO ();
	  /* Do not wrap the following literal format string with _(...).
	     For example, suppose LC_ALL is unset, LC_TIME="POSIX",
	     and LANG="ko_KR".	In that case, POSIX.2 says that LC_TIME
	     determines the format and contents of date and time strings
	     written by date, which means "date" must generate output
	     using the POSIX locale; but adding _() would cause "date"
	     to use a Korean translation of the format.  */
	  format = *date_fmt ? date_fmt : "%a %b %e %H:%M:%S %Z %Y";
	}
    }
  else if (*format == '\0')
    {
      printf ("\n");
      return;
    }

  while (1)
    {
      int done;
      out_length += 200;
      out = (char *) xrealloc (out, out_length);

      /* Mark the first byte of the buffer so we can detect the case
	 of strftime producing an empty string.  Otherwise, this loop
	 would not terminate when date was invoked like this
	 `LANG=de date +%p' on a system with good language support.  */
      out[0] = '\1';

      if (rfc_format)
	setlocale (LC_ALL, "C");

      done = (strftime (out, out_length, format, tm) || out[0] == '\0');

      if (rfc_format)
	setlocale (LC_ALL, "");

      if (done)
	break;
    }

  printf ("%s\n", out);
  free (out);
}
