/* GNU's uptime.
   Copyright (C) 1992-2002 Free Software Foundation, Inc.

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

/* Created by hacking who.c by Kaveh Ghazi ghazi@caip.rutgers.edu.  */

#include <config.h>
#include <getopt.h>
#include <stdio.h>

#include <sys/types.h>
#include "system.h"

#if HAVE_SYSCTL && HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

#include "error.h"
#include "long-options.h"
#include "readutmp.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "uptime"

#define AUTHORS N_ ("Joseph Arceneaux, David MacKenzie, and Kaveh Ghazi")

int getloadavg ();

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {NULL, 0, NULL, 0}
};

static void
print_uptime (int n, const STRUCT_UTMP *this)
{
  register int entries = 0;
  time_t boot_time = 0;
  time_t time_now;
  time_t uptime = 0;
  int updays;
  int uphours;
  int upmins;
  struct tm *tmn;
  double avg[3];
  int loads;
#ifdef HAVE_PROC_UPTIME
  FILE *fp;
  double upsecs;

  fp = fopen ("/proc/uptime", "r");
  if (fp != NULL)
    {
      char buf[BUFSIZ];
      int res;
      char *b = fgets (buf, BUFSIZ, fp);
      if (b == buf)
	{
	  /* The following sscanf must use the C locale.  */
	  setlocale (LC_NUMERIC, "C");
	  res = sscanf (buf, "%lf", &upsecs);
	  setlocale (LC_NUMERIC, "");
	  if (res == 1)
	    uptime = (time_t) upsecs;
	}

      fclose (fp);
    }
#endif /* HAVE_PROC_UPTIME */

#if HAVE_SYSCTL && defined CTL_KERN && defined KERN_BOOTTIME
  {
    /* FreeBSD specific: fetch sysctl "kern.boottime".  */
    static int request[2] = { CTL_KERN, KERN_BOOTTIME };
    struct timeval result;
    size_t result_len = sizeof result;

    if (sysctl (request, 2, &result, &result_len, NULL, 0) >= 0)
      boot_time = result.tv_sec;
  }
#endif

  /* Loop through all the utmp entries we just read and count up the valid
     ones, also in the process possibly gleaning boottime. */
  while (n--)
    {
      if (UT_USER (this) [0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
	  )
	{
	  ++entries;
	}
      /* If BOOT_MSG is defined, we can get boottime from utmp.  This avoids
	 possibly needing special privs to read /dev/kmem. */
#ifdef BOOT_MSG
# if HAVE_PROC_UPTIME
      if (uptime == 0)
# endif /* HAVE_PROC_UPTIME */
	if (STREQ (this->ut_line, BOOT_MSG))
	  boot_time = UT_TIME_MEMBER (this);
#endif /* BOOT_MSG */
      ++this;
    }
  time_now = time (0);
#if defined HAVE_PROC_UPTIME
  if (uptime == 0)
#endif
    {
      if (boot_time == 0)
	error (EXIT_FAILURE, errno, _("couldn't get boot time"));
      uptime = time_now - boot_time;
    }
  updays = uptime / 86400;
  uphours = (uptime - (updays * 86400)) / 3600;
  upmins = (uptime - (updays * 86400) - (uphours * 3600)) / 60;
  tmn = localtime (&time_now);
  printf (_(" %2d:%02d%s  up "), ((tmn->tm_hour % 12) == 0
				  ? 12 : tmn->tm_hour % 12),
	  /* FIXME: use strftime, not am, pm.  Uli reports that
	     the german translation is meaningless.  */
	  tmn->tm_min, (tmn->tm_hour < 12 ? _("am") : _("pm")));
  if (updays > 0)
    printf (ngettext("%d day", "%d days", updays), updays);
  printf (" %2d:%02d,  ", uphours, upmins);
  printf (ngettext ("%d user", "%d users", entries), entries);

#if defined (HAVE_GETLOADAVG) || defined (C_GETLOADAVG)
  loads = getloadavg (avg, 3);
#else
  loads = -1;
#endif

  if (loads == -1)
    putchar ('\n');
  else
    {
      if (loads > 0)
	printf (_(",  load average: %.2f"), avg[0]);
      if (loads > 1)
	printf (", %.2f", avg[1]);
      if (loads > 2)
	printf (", %.2f", avg[2]);
      if (loads > 0)
	putchar ('\n');
    }
}

/* Display the system uptime and the number of users on the system,
   according to utmp file FILENAME. */

static void
uptime (const char *filename)
{
  int n_users;
  STRUCT_UTMP *utmp_buf;
  int fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (EXIT_FAILURE, errno, "%s", filename);

  print_uptime (n_users, utmp_buf);
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [ FILE ]\n"), program_name);
      printf (_("\
Print the current time, the length of time the system has been up,\n\
the number of users on the system, and the average number of jobs\n\
in the run queue over the last 1, 5 and 15 minutes.\n\
If FILE is not specified, use %s.  %s as FILE is common.\n\
\n\
"),
	      UTMP_FILE, WTMP_FILE);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc, longind;
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((optc = getopt_long (argc, argv, "", longopts, &longind)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	default:
	  usage (EXIT_FAILURE);
	}
    }

  switch (argc - optind)
    {
    case 0:			/* uptime */
      uptime (UTMP_FILE);
      break;

    case 1:			/* uptime <utmp file> */
      uptime (argv[optind]);
      break;

    default:			/* lose */
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
