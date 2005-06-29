/* date - print or set the system date and time
   Copyright (C) 1989-2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

#include "system.h"
#include "argmatch.h"
#include "error.h"
#include "getdate.h"
#include "getline.h"
#include "inttostr.h"
#include "posixtm.h"
#include "quote.h"
#include "strftime.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "date"

#define AUTHORS "David MacKenzie"

int putenv ();

static bool show_date (const char *format, struct timespec when);

enum Time_spec
{
  /* display only the date: 1999-03-25 */
  TIME_SPEC_DATE=1,
  /* display date and hour: 1999-03-25T03-0500 */
  TIME_SPEC_HOURS,
  /* display date, hours, and minutes: 1999-03-25T03:23-0500 */
  TIME_SPEC_MINUTES,
  /* display date, hours, minutes, and seconds: 1999-03-25T03:23:14-0500 */
  TIME_SPEC_SECONDS,
  /* similar, but display nanoseconds: 1999-03-25T03:23:14,123456789-0500 */
  TIME_SPEC_NS
};

static char const *const time_spec_string[] =
{
  "date", "hours", "minutes", "seconds", "ns", NULL
};

static enum Time_spec const time_spec[] =
{
  TIME_SPEC_DATE, TIME_SPEC_HOURS, TIME_SPEC_MINUTES, TIME_SPEC_SECONDS,
  TIME_SPEC_NS
};

/* The name this program was run with, for error messages. */
char *program_name;

/* If nonzero, display an ISO 8601 format date/time string */
static int iso_8601_format = 0;

/* If true, display time in RFC-(2)822 format for mail or news. */
static bool rfc_format = false;

static char const short_options[] = "d:f:I::r:Rs:u";

static struct option const long_options[] =
{
  {"date", required_argument, NULL, 'd'},
  {"file", required_argument, NULL, 'f'},
  {"iso-8601", optional_argument, NULL, 'I'},
  {"reference", required_argument, NULL, 'r'},
  {"rfc-822", no_argument, NULL, 'R'},
  {"rfc-2822", no_argument, NULL, 'R'},
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
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [+FORMAT]\n\
  or:  %s [-u|--utc|--universal] [MMDDhhmm[[CC]YY][.ss]]\n\
"),
	      program_name, program_name);
      fputs (_("\
Display the current time in the given FORMAT, or set the system date.\n\
\n\
  -d, --date=STRING         display time described by STRING, not `now'\n\
  -f, --file=DATEFILE       like --date once for each line of DATEFILE\n\
  -I[TIMESPEC], --iso-8601[=TIMESPEC]  output date/time in ISO 8601 format.\n\
                            TIMESPEC=`date' for date only (the default),\n\
                            `hours', `minutes', `seconds', or `ns' for date and\n\
                            time to the indicated precision.\n\
"), stdout);
      fputs (_("\
  -r, --reference=FILE      display the last modification time of FILE\n\
  -R, --rfc-2822            output RFC-2822 compliant date string\n\
  -s, --set=STRING          set time described by STRING\n\
  -u, --utc, --universal    print or set Coordinated Universal Time\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
FORMAT controls the output.  The only valid option for the second form\n\
specifies Coordinated Universal Time.  Interpreted sequences are:\n\
\n\
  %%   a literal %\n\
  %a   locale's abbreviated weekday name (e.g., Sun)\n\
"), stdout);
      fputs (_("\
  %A   locale's full weekday name (e.g., Sunday)\n\
  %b   locale's abbreviated month name (e.g., Jan)\n\
  %B   locale's full month name (e.g., January)\n\
  %c   locale's date and time (e.g., Thu Mar  3 23:05:25 2005)\n\
"), stdout);
      fputs (_("\
  %C   century; like %Y, except omit last two digits (e.g., 21)\n\
  %d   day of month (e.g, 01)\n\
  %D   date; same as %m/%d/%y\n\
  %e   day of month, space padded; same as %_d\n\
"), stdout);
      fputs (_("\
  %F   full date; same as %Y-%m-%d\n\
  %g   the last two digits of the year corresponding to the %V week number\n\
  %G   the year corresponding to the %V week number\n\
"), stdout);
      fputs (_("\
  %h   same as %b\n\
  %H   hour (00..23)\n\
  %I   hour (01..12)\n\
  %j   day of year (001..366)\n\
"), stdout);
      fputs (_("\
  %k   hour ( 0..23)\n\
  %l   hour ( 1..12)\n\
  %m   month (01..12)\n\
  %M   minute (00..59)\n\
"), stdout);
      fputs (_("\
  %n   a newline\n\
  %N   nanoseconds (000000000..999999999)\n\
  %p   locale's equivalent of either AM or PM; blank if not known\n\
  %P   like %p, but lower case\n\
  %r   locale's 12-hour clock time (e.g., 11:11:04 PM)\n\
  %R   24-hour hour and minute; same as %H:%M\n\
  %s   seconds since 1970-01-01 00:00:00 UTC\n\
"), stdout);
      fputs (_("\
  %S   second (00..60)\n\
  %t   a tab\n\
  %T   time; same as %H:%M:%S\n\
  %u   day of week (1..7); 1 is Monday\n\
"), stdout);
      fputs (_("\
  %U   week number of year with Sunday as first day of week (00..53)\n\
  %V   week number of year with Monday as first day of week (01..53)\n\
  %w   day of week (0..6); 0 is Sunday\n\
  %W   week number of year with Monday as first day of week (00..53)\n\
"), stdout);
      fputs (_("\
  %x   locale's date representation (e.g., 12/31/99)\n\
  %X   locale's time representation (e.g., 23:13:48)\n\
  %y   last two digits of year (00..99)\n\
  %Y   year\n\
"), stdout);
      fputs (_("\
  %z   numeric timezone (e.g., -0400)\n\
  %Z   alphabetic time zone abbreviation (e.g., EDT)\n\
\n\
By default, date pads numeric fields with zeroes.\n\
The following optional flags may follow `%':\n\
\n\
  - (hyphen) do not pad the field\n\
  _ (underscore) pad with spaces\n\
  0 (zero) pad with zeros\n\
  ^ use upper case if possible\n\
  # use opposite case if possible\n\
"), stdout);
      fputs (_("\
\n\
After any flags comes an optional field width, as a decimal number;\n\
then an optional modifier, which is either\n\
E to use the locale's alternate representations if available, or\n\
O to use the locale's alternate numeric symbols if available.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Parse each line in INPUT_FILENAME as with --date and display each
   resulting time and date.  If the file cannot be opened, tell why
   then exit.  Issue a diagnostic for any lines that cannot be parsed.
   Return true if successful.  */

static bool
batch_convert (const char *input_filename, const char *format)
{
  bool ok;
  FILE *in_stream;
  char *line;
  size_t buflen;
  struct timespec when;

  if (STREQ (input_filename, "-"))
    {
      input_filename = _("standard input");
      in_stream = stdin;
    }
  else
    {
      in_stream = fopen (input_filename, "r");
      if (in_stream == NULL)
	{
	  error (EXIT_FAILURE, errno, "%s", quote (input_filename));
	}
    }

  line = NULL;
  buflen = 0;
  ok = true;
  while (1)
    {
      ssize_t line_length = getline (&line, &buflen, in_stream);
      if (line_length < 0)
	{
	  /* FIXME: detect/handle error here.  */
	  break;
	}

      if (! get_date (&when, line, NULL))
	{
	  if (line[line_length - 1] == '\n')
	    line[line_length - 1] = '\0';
	  error (0, 0, _("invalid date %s"), quote (line));
	  ok = false;
	}
      else
	{
	  ok &= show_date (format, when);
	}
    }

  if (fclose (in_stream) == EOF)
    error (EXIT_FAILURE, errno, "%s", quote (input_filename));

  free (line);

  return ok;
}

int
main (int argc, char **argv)
{
  int optc;
  const char *datestr = NULL;
  const char *set_datestr = NULL;
  struct timespec when;
  bool set_date = false;
  char *format;
  char *batch_file = NULL;
  char *reference = NULL;
  struct stat refstats;
  int n_args;
  bool ok;
  int option_specified_date;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    switch (optc)
      {
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
	rfc_format = true;
	break;
      case 's':
	set_datestr = optarg;
	set_date = true;
	break;
      case 'u':
	/* POSIX says that `date -u' is equivalent to setting the TZ
	   environment variable, so this option should do nothing other
	   than setting TZ.  */
	if (putenv ("TZ=UTC0") != 0)
	  xalloc_die ();
	TZSET;
	break;
      case_GETOPT_HELP_CHAR;
      case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
      default:
	usage (EXIT_FAILURE);
      }

  n_args = argc - optind;

  option_specified_date = ((datestr ? 1 : 0)
			   + (batch_file ? 1 : 0)
			   + (reference ? 1 : 0));

  if (option_specified_date > 1)
    {
      error (0, 0,
	_("the options to specify dates for printing are mutually exclusive"));
      usage (EXIT_FAILURE);
    }

  if (set_date && option_specified_date)
    {
      error (0, 0,
	  _("the options to print and set the time may not be used together"));
      usage (EXIT_FAILURE);
    }

  if (n_args > 1)
    {
      error (0, 0, _("extra operand %s"), quote (argv[optind + 1]));
      usage (EXIT_FAILURE);
    }

  if ((set_date || option_specified_date)
      && n_args == 1 && argv[optind][0] != '+')
    {
      error (0, 0, _("\
the argument %s lacks a leading `+';\n\
When using an option to specify date(s), any non-option\n\
argument must be a format string beginning with `+'."),
	     quote (argv[optind]));
      usage (EXIT_FAILURE);
    }

  /* Simply ignore --rfc-2822 if specified when setting the date.  */
  if (rfc_format && !set_date && n_args > 0)
    {
      error (0, 0,
	     _("a format string may not be specified when using\
 the --rfc-2822 (-R) option"));
      usage (EXIT_FAILURE);
    }

  if (set_date)
    datestr = set_datestr;

  if (batch_file != NULL)
    ok = batch_convert (batch_file, (n_args == 1 ? argv[optind] + 1 : NULL));
  else
    {
      bool valid_date = true;
      ok = true;

      if (!option_specified_date && !set_date)
	{
	  if (n_args == 1 && argv[optind][0] != '+')
	    {
	      /* Prepare to set system clock to the specified date/time
		 given in the POSIX-format.  */
	      set_date = true;
	      datestr = argv[optind];
	      valid_date = posixtime (&when.tv_sec,
				      datestr,
				      (PDS_TRAILING_YEAR
				       | PDS_CENTURY | PDS_SECONDS));
	      when.tv_nsec = 0; /* FIXME: posixtime should set this.  */
	      format = NULL;
	    }
	  else
	    {
	      /* Prepare to print the current date/time.  */
	      datestr = _("undefined");
	      gettime (&when);
	      format = (n_args == 1 ? argv[optind] + 1 : NULL);
	    }
	}
      else
	{
	  /* (option_specified_date || set_date) */
	  if (reference != NULL)
	    {
	      if (stat (reference, &refstats))
		error (EXIT_FAILURE, errno, "%s", reference);
	      when.tv_sec = refstats.st_mtime;
	      when.tv_nsec = TIMESPEC_NS (refstats.st_mtim);
	    }
	  else
	    {
	      valid_date = get_date (&when, datestr, NULL);
	    }

	  format = (n_args == 1 ? argv[optind] + 1 : NULL);
	}

      if (! valid_date)
	error (EXIT_FAILURE, 0, _("invalid date %s"), quote (datestr));

      if (set_date)
	{
	  /* Set the system clock to the specified date, then regardless of
	     the success of that operation, format and print that date.  */
	  if (settime (&when) != 0)
	    {
	      error (0, errno, _("cannot set date"));
	      ok = false;
	    }
	}

      ok &= show_date (format, when);
    }

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Display the date and/or time in WHEN according to the format specified
   in FORMAT, followed by a newline.  If FORMAT is NULL, use the
   standard output format (ctime style but with a timezone inserted).
   Return true if successful.  */

static bool
show_date (const char *format, struct timespec when)
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
    "%Y-%m-%dT%H:%M:%S%z",
    "%Y-%m-%dT%H:%M:%S,%N%z"
  };

  if (format == NULL)
    {
      if (rfc_format)
	format = "%a, %d %b %Y %H:%M:%S %z";
      else if (iso_8601_format)
	format = iso_format_string[iso_8601_format - 1];
      else
	{
	  char *date_fmt = DATE_FMT_LANGINFO ();
	  /* Do not wrap the following literal format string with _(...).
	     For example, suppose LC_ALL is unset, LC_TIME="POSIX",
	     and LANG="ko_KR".	In that case, POSIX says that LC_TIME
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
      return true;
    }

  tm = localtime (&when.tv_sec);
  if (! tm)
    {
      char buf[INT_BUFSIZE_BOUND (intmax_t)];
      error (0, 0, _("time %s is out of range"),
	     (TYPE_SIGNED (time_t)
	      ? imaxtostr (when.tv_sec, buf)
	      : umaxtostr (when.tv_sec, buf)));
      puts (buf);
      return false;
    }

  while (1)
    {
      bool done;
      out = X2REALLOC (out, &out_length);

      /* Mark the first byte of the buffer so we can detect the case
	 of nstrftime producing an empty string.  Otherwise, this loop
	 would not terminate when date was invoked like this
	 `LANG=de date +%p' on a system with good language support.  */
      out[0] = '\1';

      if (rfc_format)
	setlocale (LC_ALL, "C");

      done = (nstrftime (out, out_length, format, tm, 0, when.tv_nsec)
	      || out[0] == '\0');

      if (rfc_format)
	setlocale (LC_ALL, "");

      if (done)
	break;
    }

  puts (out);
  free (out);
  return true;
}
