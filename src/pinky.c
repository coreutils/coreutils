/* GNU's pinky.
   Copyright (C) 1992-1997, 1999-2002 Free Software Foundation, Inc.

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

/* Created by hacking who.c by Kaveh Ghazi ghazi@caip.rutgers.edu */

#include <config.h>
#include <getopt.h>
#include <pwd.h>
#include <stdio.h>

#include <sys/types.h>
#include "system.h"

#include "error.h"
#include "readutmp.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "pinky"

#define AUTHORS N_ ("Joseph Arceneaux, David MacKenzie, and Kaveh Ghazi")

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#ifndef S_IWGRP
# define S_IWGRP 020
#endif

int gethostname ();
char *ttyname ();

/* The name this program was run with. */
const char *program_name;

/* If nonzero, display the hours:minutes since each user has touched
   the keyboard, or blank if within the last minute, or days followed
   by a 'd' if not within the last day. */
static int include_idle = 1;

/* If nonzero, display a line at the top describing each field. */
static int include_heading = 1;

/* if nonzero, display the user's full name from pw_gecos. */
static int include_fullname = 1;

/* if nonzero, display the user's ~/.project file when doing long format. */
static int include_project = 1;

/* if nonzero, display the user's ~/.plan file when doing long format. */
static int include_plan = 1;

/* if nonzero, display the user's home directory and shell
   when doing long format. */
static int include_home_and_shell = 1;

/* if nonzero, use the "short" output format. */
static int do_short_format = 1;

/* if nonzero, display the ut_host field. */
#ifdef HAVE_UT_HOST
static int include_where = 1;
#endif

static struct option const longopts[] =
{
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Count and return the number of ampersands in STR.  */

static int
count_ampersands (const char *str)
{
  int count = 0;
  do
    {
      if (*str == '&')
	count++;
    } while (*str++);
  return count;
}

/* Create a string (via xmalloc) which contains a full name by substituting
   for each ampersand in GECOS_NAME the USER_NAME string with its first
   character capitalized.  The caller must ensure that GECOS_NAME contains
   no `,'s.  The caller also is responsible for free'ing the return value of
   this function.  */

static char *
create_fullname (const char *gecos_name, const char *user_name)
{
  const int result_len = strlen (gecos_name)
    + count_ampersands (gecos_name) * (strlen (user_name) - 1) + 1;
  char *const result = xmalloc (result_len);
  char *r = result;

  while (*gecos_name)
    {
      if (*gecos_name == '&')
	{
	  const char *uname = user_name;
	  if (ISLOWER (*uname))
	    *r++ = TOUPPER (*uname++);
	  while (*uname)
	    *r++ = *uname++;
	}
      else
	{
	  *r++ = *gecos_name;
	}

      gecos_name++;
    }
  *r = 0;

  return result;
}

/* Return a string representing the time between WHEN and the time
   that this function is first run. */

static const char *
idle_string (time_t when)
{
  static time_t now = 0;
  static char idle_hhmm[10];
  time_t seconds_idle;

  if (now == 0)
    time (&now);

  seconds_idle = now - when;
  if (seconds_idle < 60)	/* One minute. */
    return "     ";
  if (seconds_idle < (24 * 60 * 60))	/* One day. */
    {
      sprintf (idle_hhmm, "%02d:%02d",
	       (int) (seconds_idle / (60 * 60)),
	       (int) ((seconds_idle % (60 * 60)) / 60));
      return (const char *) idle_hhmm;
    }
  sprintf (idle_hhmm, "%dd", (int) (seconds_idle / (24 * 60 * 60)));
  return (const char *) idle_hhmm;
}

/* Display a line of information about UTMP_ENT. */

static void
print_entry (const STRUCT_UTMP *utmp_ent)
{
  struct stat stats;
  time_t last_change;
  char mesg;

#define DEV_DIR_WITH_TRAILING_SLASH "/dev/"
#define DEV_DIR_LEN (sizeof (DEV_DIR_WITH_TRAILING_SLASH) - 1)

  char line[sizeof (utmp_ent->ut_line) + DEV_DIR_LEN + 1];
  time_t tm;

  /* Copy ut_line into LINE, prepending `/dev/' if ut_line is not
     already an absolute pathname.  Some system may put the full,
     absolute pathname in ut_line.  */
  if (utmp_ent->ut_line[0] == '/')
    {
      strncpy (line, utmp_ent->ut_line, sizeof (utmp_ent->ut_line));
      line[sizeof (utmp_ent->ut_line)] = '\0';
    }
  else
    {
      strcpy (line, DEV_DIR_WITH_TRAILING_SLASH);
      strncpy (line + DEV_DIR_LEN, utmp_ent->ut_line, sizeof (utmp_ent->ut_line));
      line[DEV_DIR_LEN + sizeof (utmp_ent->ut_line)] = '\0';
    }

  if (stat (line, &stats) == 0)
    {
      mesg = (stats.st_mode & S_IWGRP) ? ' ' : '*';
      last_change = stats.st_atime;
    }
  else
    {
      mesg = '?';
      last_change = 0;
    }

  printf ("%-8.*s", (int) sizeof (UT_USER (utmp_ent)), UT_USER (utmp_ent));

  if (include_fullname)
    {
      struct passwd *pw;
      char name[sizeof (UT_USER (utmp_ent)) + 1];

      strncpy (name, UT_USER (utmp_ent), sizeof (UT_USER (utmp_ent)));
      name[sizeof (UT_USER (utmp_ent))] = '\0';
      pw = getpwnam (name);
      if (pw == NULL)
	printf (" %19s", "        ???");
      else
	{
	  char *const comma = strchr (pw->pw_gecos, ',');
	  char *result;

	  if (comma)
	    *comma = '\0';

	  result = create_fullname (pw->pw_gecos, pw->pw_name);
	  printf (" %-19.19s", result);
	  free (result);
	}
    }

  printf (" %c%-8.*s",
	  mesg, (int) sizeof (utmp_ent->ut_line), utmp_ent->ut_line);

  if (include_idle)
    {
      if (last_change)
	printf (" %-6s", idle_string (last_change));
      else
	printf (" %-6s", "???");
    }

  /* Don't take the address of UT_TIME_MEMBER directly.
     Ulrich Drepper wrote:
     ``... GNU libc (and perhaps other libcs as well) have extended
     utmp file formats which do not use a simple time_t ut_time field.
     In glibc, ut_time is a macro which selects for backward compatibility
     the tv_sec member of a struct timeval value.''  */
  tm = UT_TIME_MEMBER (utmp_ent);
  printf (" %-12.12s", ctime (&tm) + 4);

#ifdef HAVE_UT_HOST
  if (include_where && utmp_ent->ut_host[0])
    {
      extern char *canon_host ();
      char ut_host[sizeof (utmp_ent->ut_host) + 1];
      char *host = 0, *display = 0;

      /* Copy the host name into UT_HOST, and ensure it's nul terminated. */
      strncpy (ut_host, utmp_ent->ut_host, (int) sizeof (utmp_ent->ut_host));
      ut_host[sizeof (utmp_ent->ut_host)] = '\0';

      /* Look for an X display.  */
      display = strrchr (ut_host, ':');
      if (display)
	*display++ = '\0';

      if (*ut_host)
	/* See if we can canonicalize it.  */
	host = canon_host (ut_host);
      if ( ! host)
	host = ut_host;

      if (display)
	printf (" %s:%s", host, display);
      else
	printf (" %s", host);
    }
#endif

  putchar ('\n');
}

/* Display a verbose line of information about UTMP_ENT. */

static void
print_long_entry (const char name[])
{
  struct passwd *pw;

  pw = getpwnam (name);

  printf (_("Login name: "));
  printf ("%-28s", name);

  printf (_("In real life: "));
  if (pw == NULL)
    {
      printf (" %s", _("???\n"));
      return;
    }
  else
    {
      char *const comma = strchr (pw->pw_gecos, ',');
      char *result;

      if (comma)
	*comma = '\0';

      result = create_fullname (pw->pw_gecos, pw->pw_name);
      printf (" %s", result);
      free (result);
    }

  putchar ('\n');

  if (include_home_and_shell)
    {
      printf (_("Directory: "));
      printf ("%-29s", pw->pw_dir);
      printf (_("Shell: "));
      printf (" %s", pw->pw_shell);
      putchar ('\n');
    }

  if (include_project)
    {
      FILE *stream;
      char buf[1024];
      const char *const baseproject = "/.project";
      char *const project =
      xmalloc (strlen (pw->pw_dir) + strlen (baseproject) + 1);

      strcpy (project, pw->pw_dir);
      strcat (project, baseproject);

      stream = fopen (project, "r");
      if (stream)
	{
	  size_t bytes;

	  printf (_("Project: "));

	  while ((bytes = fread (buf, 1, sizeof (buf), stream)) > 0)
	    fwrite (buf, 1, bytes, stdout);
	  fclose (stream);
	}

      free (project);
    }

  if (include_plan)
    {
      FILE *stream;
      char buf[1024];
      const char *const baseplan = "/.plan";
      char *const plan =
      xmalloc (strlen (pw->pw_dir) + strlen (baseplan) + 1);

      strcpy (plan, pw->pw_dir);
      strcat (plan, baseplan);

      stream = fopen (plan, "r");
      if (stream)
	{
	  size_t bytes;

	  printf (_("Plan:\n"));

	  while ((bytes = fread (buf, 1, sizeof (buf), stream)) > 0)
	    fwrite (buf, 1, bytes, stdout);
	  fclose (stream);
	}

      free (plan);
    }

  putchar ('\n');
}

/* Print the username of each valid entry and the number of valid entries
   in UTMP_BUF, which should have N elements. */

static void
print_heading (void)
{
  printf ("%-8s", _("Login"));
  if (include_fullname)
    printf (" %-19s", _("Name"));
  printf (" %-9s", _(" TTY"));
  if (include_idle)
    printf (" %-6s", _("Idle"));
  printf (" %-12s", _("When"));
#ifdef HAVE_UT_HOST
  if (include_where)
    printf (" %s", _("Where"));
#endif
  putchar ('\n');
}

/* Display UTMP_BUF, which should have N entries. */

static void
scan_entries (int n, const STRUCT_UTMP *utmp_buf,
	      const int argc_names, char *const argv_names[])
{
  if (include_heading)
    print_heading ();

  while (n--)
    {
      if (UT_USER (utmp_buf)[0]
#ifdef USER_PROCESS
	  && utmp_buf->ut_type == USER_PROCESS
#endif
	)
	{
	  if (argc_names)
	    {
	      int i;

	      for (i = 0; i < argc_names; i++)
		if (strncmp (UT_USER (utmp_buf), argv_names[i],
			     sizeof (UT_USER (utmp_buf))) == 0)
		  {
		    print_entry (utmp_buf);
		    break;
		  }
	    }
	  else
	    print_entry (utmp_buf);
	}
      utmp_buf++;
    }
}

/* Display a list of who is on the system, according to utmp file FILENAME. */

static void
short_pinky (const char *filename,
	     const int argc_names, char *const argv_names[])
{
  int n_users;
  STRUCT_UTMP *utmp_buf;
  int fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (EXIT_FAILURE, errno, "%s", filename);

  scan_entries (n_users, utmp_buf, argc_names, argv_names);
}

static void
long_pinky (const int argc_names, char *const argv_names[])
{
  int i;

  for (i = 0; i < argc_names; i++)
    print_long_entry (argv_names[i]);
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [USER]...\n"), program_name);
      fputs (_("\
\n\
  -l              produce long format output for the specified USERs\n\
  -b              omit the user's home directory and shell in long format\n\
  -h              omit the user's project file in long format\n\
  -p              omit the user's plan file in long format\n\
  -s              do short format output, this is the default\n\
"), stdout);
      fputs (_("\
  -f              omit the line of column headings in short format\n\
  -w              omit the user's full name in short format\n\
  -i              omit the user's full name and remote host in short format\n\
  -q              omit the user's full name, remote host and idle time\n\
                  in short format\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\
\n\
A lightweight `finger' program;  print user information.\n\
The utmp file will be %s.\n\
"), UTMP_FILE);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc, longind;
  int n_users;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "sfwiqbhlp", longopts, &longind))
	 != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 's':
	  do_short_format = 1;
	  break;

	case 'l':
	  do_short_format = 0;
	  break;

	case 'f':
	  include_heading = 0;
	  break;

	case 'w':
	  include_fullname = 0;
	  break;

	case 'i':
	  include_fullname = 0;
#ifdef HAVE_UT_HOST
	  include_where = 0;
#endif
	  break;

	case 'q':
	  include_fullname = 0;
#ifdef HAVE_UT_HOST
	  include_where = 0;
#endif
	  include_idle = 0;
	  break;

	case 'h':
	  include_project = 0;
	  break;

	case 'p':
	  include_plan = 0;
	  break;

	case 'b':
	  include_home_and_shell = 0;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  n_users = argc - optind;

  if (do_short_format == 0 && n_users == 0)
    {
      error (0, 0, _("no username specified; at least one must be\
 specified when using -l"));
      usage (EXIT_FAILURE);
    }

  if (do_short_format)
    short_pinky (UTMP_FILE, n_users, argv + optind);
  else
    long_pinky (n_users, argv + optind);

  exit (EXIT_SUCCESS);
}
