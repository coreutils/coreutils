/* touch -- change modification and access times of files
   Copyright (C) 87, 1989-1991, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Paul Rubin, Arnold Robbins, Jim Kingdon, David MacKenzie,
   and Randy Smith. */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "argmatch.h"
#include "error.h"
#include "getdate.h"
#include "posixtm.h"
#include "posixver.h"
#include "quote.h"
#include "safe-read.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "touch"

#define AUTHORS \
N_ ("Paul Rubin, Arnold Robbins, Jim Kingdon, David MacKenzie, and Randy Smith")

#ifndef STDC_HEADERS
time_t time ();
#endif

/* Bitmasks for `change_times'. */
#define CH_ATIME 1
#define CH_MTIME 2

#if !defined O_NDELAY
# define O_NDELAY 0
#endif

#if !defined O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif

#if !defined O_NOCTTY
# define O_NOCTTY 0
#endif

#if !defined EISDIR
# define EISDIR 0
#endif

/* The name by which this program was run. */
char *program_name;

/* Which timestamps to change. */
static int change_times;

/* (-c) If nonzero, don't create if not already there. */
static int no_create;

/* (-d) If nonzero, date supplied on command line in get_date formats. */
static int flexible_date;

/* (-r) If nonzero, use times from a reference file. */
static int use_ref;

/* (-t) If nonzero, date supplied on command line in POSIX format. */
static int posix_date;

/* If nonzero, the only thing we have to do is change both the
   modification and access time to the current time, so we don't
   have to own the file, just be able to read and write it.
   On some systems, we can do this if we own the file, even though
   we have neither read nor write access to it.  */
static int amtime_now;

/* New time to use when setting time. */
static time_t newtime;

/* File to use for -r. */
static char *ref_file;

/* Info about the reference file. */
static struct stat ref_stats;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TIME_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  {"time", required_argument, 0, TIME_OPTION},
  {"no-create", no_argument, 0, 'c'},
  {"date", required_argument, 0, 'd'},
  {"file", required_argument, 0, 'r'}, /* FIXME: phase out --file */
  {"reference", required_argument, 0, 'r'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0}
};

/* Valid arguments to the `--time' option. */
static char const* const time_args[] =
{
  "atime", "access", "use", "mtime", "modify", 0
};

/* The bits in `change_times' that those arguments set. */
static int const time_masks[] =
{
  CH_ATIME, CH_ATIME, CH_ATIME, CH_MTIME, CH_MTIME
};

/* Update the time of file FILE according to the options given.
   Return 0 if successful, 1 if an error occurs. */

static int
touch (const char *file)
{
  int status;
  struct stat sbuf;
  int fd = -1;
  int open_errno = 0;

  if (! no_create)
    {
      /* Try to open FILE, creating it if necessary.  */
      fd = open (file, O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

      /* Don't save a copy of errno if it's EISDIR, since that would lead
	 touch to give a bogus diagnostic for e.g., `touch /' (assuming
	 we don't own / or have write access to it).  On Solaris 5.6,
	 and probably other systems, it is EINVAL.  On SunOS4, it's EPERM.  */
      if (fd == -1 && errno != EISDIR && errno != EINVAL && errno != EPERM)
	open_errno = errno;
    }

  if (! amtime_now)
    {
      /* We're setting only one of the time values.  stat the target to get
	 the other one.  If we have the file descriptor already, use fstat.
	 Otherwise, either we're in no-create mode (and hence didn't call open)
	 or FILE is inaccessible or a directory, so we have to use stat.  */
      if (fd != -1 ? fstat (fd, &sbuf) : stat (file, &sbuf))
	{
	  if (open_errno)
	    error (0, open_errno, _("creating %s"), quote (file));
	  else
	    {
	      if (no_create && errno == ENOENT)
		return 0;
	      error (0, errno, _("failed to get attributes of %s"),
		     quote (file));
	    }
	  close (fd);
	  return 1;
	}
    }

  if (fd != -1 && close (fd) < 0)
    {
      error (0, errno, _("creating %s"), quote (file));
      return 1;
    }

  if (amtime_now)
    {
      /* Pass NULL to utime so it will not fail if we just have
	 write access to the file, but don't own it.  */
      status = utime (file, NULL);
    }
  else
    {
      struct utimbuf utb;

      /* There's currently no interface to set file timestamps with
	 better than 1-second resolution, so discard any fractional
	 part of the source timestamp.  */

      if (use_ref)
	{
	  utb.actime = ref_stats.st_atime;
	  utb.modtime = ref_stats.st_mtime;
	}
      else
	utb.actime = utb.modtime = newtime;

      if (!(change_times & CH_ATIME))
	utb.actime = sbuf.st_atime;

      if (!(change_times & CH_MTIME))
	utb.modtime = sbuf.st_mtime;

      status = utime (file, &utb);
    }

  if (status)
    {
      if (open_errno)
	{
	  /* The wording of this diagnostic should cover at least two cases:
	     - the file does not exist, but the parent directory is unwritable
	     - the file exists, but it isn't writable
	     I think it's not worth trying to distinguish them.  */
	  error (0, open_errno, _("cannot touch %s"), quote (file));
	}
      else
	{
	  if (no_create && errno == ENOENT)
	    return 0;
	  error (0, errno, _("setting times of %s"), quote (file));
	}
      return 1;
    }

  return 0;
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Update the access and modification times of each FILE to the current time.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -a                     change only the access time\n\
  -c, --no-create        do not create any files\n\
  -d, --date=STRING      parse STRING and use it instead of current time\n\
  -f                     (ignored)\n\
  -m                     change only the modification time\n\
"), stdout);
      fputs (_("\
  -r, --reference=FILE   use this file's times instead of current time\n\
  -t STAMP               use [[CC]YY]MMDDhhmm[.ss] instead of current time\n\
  --time=WORD            set time given by WORD: access atime use (same as -a)\n\
                           modify mtime (same as -m)\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Note that the -d and -t options accept different time-date formats.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;
  int date_set = 0;
  int err = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  change_times = no_create = use_ref = posix_date = flexible_date = 0;

  while ((c = getopt_long (argc, argv, "acd:fmr:t:", longopts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'a':
	  change_times |= CH_ATIME;
	  break;

	case 'c':
	  no_create++;
	  break;

	case 'd':
	  flexible_date++;
	  newtime = get_date (optarg, NULL);
	  if (newtime == (time_t) -1)
	    error (EXIT_FAILURE, 0, _("invalid date format %s"), quote (optarg));
	  date_set++;
	  break;

	case 'f':
	  break;

	case 'm':
	  change_times |= CH_MTIME;
	  break;

	case 'r':
	  use_ref++;
	  ref_file = optarg;
	  break;

	case 't':
	  posix_date++;
	  if (! posixtime (&newtime, optarg,
			   PDS_LEADING_YEAR | PDS_CENTURY | PDS_SECONDS))
	    error (EXIT_FAILURE, 0, _("invalid date format %s"), quote (optarg));
	  date_set++;
	  break;

	case TIME_OPTION:	/* --time */
	  change_times |= XARGMATCH ("--time", optarg,
				     time_args, time_masks);
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (change_times == 0)
    change_times = CH_ATIME | CH_MTIME;

  if ((use_ref && (posix_date || flexible_date))
      || (posix_date && flexible_date))
    {
      error (0, 0, _("cannot specify times from more than one source"));
      usage (EXIT_FAILURE);
    }

  if (use_ref)
    {
      if (stat (ref_file, &ref_stats))
	error (EXIT_FAILURE, errno,
	       _("failed to get attributes of %s"), quote (ref_file));
      date_set++;
    }

  /* The obsolete `MMDDhhmm[YY]' form is valid IFF there are
     two or more non-option arguments.  */
  if (!date_set && 2 <= argc - optind && !STREQ (argv[optind - 1], "--")
      && posix2_version () < 200112)
    {
      if (posixtime (&newtime, argv[optind], PDS_TRAILING_YEAR))
	{
	  if (! getenv ("POSIXLY_CORRECT"))
	    {
	      struct tm const *tm = localtime (&newtime);
	      error (0, 0,
		     _("warning: `touch %s' is obsolete; use\
 `touch -t %04d%02d%02d%02d%02d.%02d'"),
		     argv[optind],
		     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		     tm->tm_hour, tm->tm_min, tm->tm_sec);
	    }

	  optind++;
	  date_set++;
	}
    }
  if (!date_set)
    {
      if ((change_times & (CH_ATIME | CH_MTIME)) == (CH_ATIME | CH_MTIME))
	amtime_now = 1;
      else
	time (&newtime);
    }

  if (optind == argc)
    {
      error (0, 0, _("file arguments missing"));
      usage (EXIT_FAILURE);
    }

  for (; optind < argc; ++optind)
    err += touch (argv[optind]);

  exit (err != 0);
}
