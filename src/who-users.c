/* GNU's uptime/users/who.
   Copyright (C) 92, 93, 94, 95, 1996 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm */

/* Output format:
   name [state] line time [idle] host
   state: -T
   name, line, time: not -q
   idle: -u
*/

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_UTMPX_H
# include <utmpx.h>
# define STRUCT_UTMP struct utmpx
# define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_tv.tv_sec)
#else
# include <utmp.h>
# define STRUCT_UTMP struct utmp
# define UT_TIME_MEMBER(UT_PTR) ((UT_PTR)->ut_time)
#endif

#include <time.h>
#include <getopt.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#include "system.h"
#include "error.h"

#if !defined (UTMP_FILE) && defined (_PATH_UTMP)
# define UTMP_FILE _PATH_UTMP
#endif

#if !defined (WTMP_FILE) && defined (_PATH_WTMP)
# define WTMP_FILE _PATH_WTMP
#endif

#ifdef UTMPX_FILE /* Solaris, SysVr4 */
# undef UTMP_FILE
# define UTMP_FILE UTMPX_FILE
#endif

#ifdef WTMPX_FILE /* Solaris, SysVr4 */
# undef WTMP_FILE
# define WTMP_FILE WTMPX_FILE
#endif

#ifndef UTMP_FILE
# define UTMP_FILE "/etc/utmp"
#endif

#ifndef WTMP_FILE
# define WTMP_FILE "/etc/wtmp"
#endif

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#ifndef S_IWGRP
# define S_IWGRP 020
#endif

#ifdef WHO
# define COMMAND_NAME "who"
#else
# ifdef USERS
#  define COMMAND_NAME "users"
# else
#  ifdef UPTIME
#   define COMMAND_NAME "uptime"
#  else
 error You must define one of WHO, UPTIME or USERS.
#  endif /* UPTIME */
# endif /* USERS */
#endif /* WHO */

int gethostname ();
char *ttyname ();
char *xmalloc ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

#ifdef WHO
/* If nonzero, display only a list of usernames and count of
   the users logged on.
   Ignored for `who am i'. */
static int short_list;

/* If nonzero, display the hours:minutes since each user has touched
   the keyboard, or "." if within the last minute, or "old" if
   not within the last day. */
static int include_idle;

/* If nonzero, display a line at the top describing each field. */
static int include_heading;

/* If nonzero, display a `+' for each user if mesg y, a `-' if mesg n,
   or a `?' if their tty cannot be statted. */
static int include_mesg;
#endif /* WHO */

static struct option const longopts[] =
{
#ifdef WHO
  {"count", no_argument, NULL, 'q'},
  {"idle", no_argument, NULL, 'u'},
  {"heading", no_argument, NULL, 'H'},
  {"message", no_argument, NULL, 'T'},
  {"mesg", no_argument, NULL, 'T'},
  {"writable", no_argument, NULL, 'T'},
#endif /* WHO */
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static STRUCT_UTMP *utmp_contents;

#ifdef UPTIME
static void
print_uptime (int n)
{
  register STRUCT_UTMP *this = utmp_contents;
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
# ifdef HAVE_PROC_UPTIME
  FILE *fp;
  double upsecs;

  fp = fopen ("/proc/uptime", "r");
  if (fp != NULL)
    {
      char buf[BUFSIZ];
      int res;
      fgets(buf, BUFSIZ, fp);
      res = sscanf (buf, "%lf", &upsecs);
      if (res == 1)
	uptime = (time_t) upsecs;
      fclose (fp);
    }
# endif /* HAVE_PROC_UPTIME */
  /* Loop through all the utmp entries we just read and count up the valid
     ones, also in the process possibly gleaning boottime. */
  while (n--)
    {
      if (this->ut_name[0]
# ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
# endif
	  )
	{
	  ++entries;
	}
      /* If BOOT_MSG is defined, we can get boottime from utmp.  This avoids
	 possibly needing special privs to read /dev/kmem. */
# ifdef BOOT_MSG
#  if HAVE_PROC_UPTIME
      if (uptime == 0)
#  endif /* HAVE_PROC_UPTIME */
	if (!strcmp (this->ut_line, BOOT_MSG))
	  boot_time = UT_TIME_MEMBER (this);
# endif /* BOOT_MSG */
      ++this;
  }
  time_now = time (0);
# if defined HAVE_PROC_UPTIME
  if (uptime == 0)
# endif
    {
      if (boot_time == 0)
	error (1, errno, _("couldn't get boot time"));
      uptime = time_now - boot_time;
    }
  updays = uptime / 86400;
  uphours = (uptime - (updays * 86400)) / 3600;
  upmins = (uptime - (updays * 86400) - (uphours * 3600)) / 60;
  tmn = localtime (&time_now);
  printf (_(" %2d:%02d%s  up "), ((tmn->tm_hour % 12) == 0
			       ? 12 : tmn->tm_hour % 12),
	  tmn->tm_min, (tmn->tm_hour < 12 ? _("am") : _("pm")));
  if (updays > 0)
    printf ("%d %s,", updays, (updays == 1 ? _("day") : _("days")));
  printf (" %2d:%02d,  %d %s", uphours, upmins, entries,
	  (entries == 1) ? _("user") : _("users"));

# if defined (HAVE_GETLOADAVG) || defined (C_GETLOADAVG)
  loads = getloadavg (avg, 3);
# else
  loads = -1;
# endif

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
#endif /* UPTIME */



#if defined (WHO) || defined (USERS)

/* Copy UT->ut_name into storage obtained from malloc.  Then remove any
   trailing spaces from the copy, NUL terminate it, and return the copy.  */

static char *
extract_trimmed_name (const STRUCT_UTMP *ut)
{
  char *p, *trimmed_name;

  trimmed_name = xmalloc (sizeof (ut->ut_name) + 1);
  strncpy (trimmed_name, ut->ut_name, sizeof (ut->ut_name));
  /* Append a trailing space character.  Some systems pad names shorter than
     the maximum with spaces, others pad with NULs.  Remove any spaces.  */
  trimmed_name[sizeof (ut->ut_name)] = ' ';
  p = strchr (trimmed_name, ' ');
  if (p != NULL)
    *p = '\0';
  return trimmed_name;
}

#endif /* WHO || USERS */

#if WHO

/* Return a string representing the time between WHEN and the time
   that this function is first run. */

static const char *
idle_string (time_t when)
{
  static time_t now = 0;
  static char idle[10];
  time_t seconds_idle;

  if (now == 0)
    time (&now);

  seconds_idle = now - when;
  if (seconds_idle < 60)	/* One minute. */
    return "  .  ";
  if (seconds_idle < (24 * 60 * 60)) /* One day. */
    {
      sprintf (idle, "%02d:%02d",
	       (int) (seconds_idle / (60 * 60)),
	       (int) ((seconds_idle % (60 * 60)) / 60));
      return (const char *) idle;
    }
  return _(" old ");
}

/* Display a line of information about entry THIS. */

static void
print_entry (STRUCT_UTMP *this)
{
  struct stat stats;
  time_t last_change;
  char mesg;

# define DEV_DIR_WITH_TRAILING_SLASH "/dev/"
# define DEV_DIR_LEN (sizeof (DEV_DIR_WITH_TRAILING_SLASH) - 1)

  char line[sizeof (this->ut_line) + DEV_DIR_LEN + 1];
  time_t tm;

  /* Copy ut_line into LINE, prepending `/dev/' if ut_line is not
     already an absolute pathname.  Some system may put the full,
     absolute pathname in ut_line.  */
  if (this->ut_line[0] == '/')
    {
      strncpy (line, this->ut_line, sizeof (this->ut_line));
      line[sizeof (this->ut_line)] = '\0';
    }
  else
    {
      strcpy (line, DEV_DIR_WITH_TRAILING_SLASH);
      strncpy (line + DEV_DIR_LEN, this->ut_line, sizeof (this->ut_line));
      line[DEV_DIR_LEN + sizeof (this->ut_line)] = '\0';
    }

  if (stat (line, &stats) == 0)
    {
      mesg = (stats.st_mode & S_IWGRP) ? '+' : '-';
      last_change = stats.st_atime;
    }
  else
    {
      mesg = '?';
      last_change = 0;
    }

  printf ("%-8.*s", (int) sizeof (this->ut_name), this->ut_name);
  if (include_mesg)
    printf ("  %c  ", mesg);
  printf (" %-8.*s", (int) sizeof (this->ut_line), this->ut_line);

  /* Don't take the address of UT_TIME_MEMBER directly.
     Ulrich Drepper wrote:
     ``... GNU libc (and perhaps other libcs as well) have extended
     utmp file formats which do not use a simple time_t ut_time field.
     In glibc, ut_time is a macro which selects for backward compatibility
     the tv_sec member of a struct timeval value.''  */
  tm = UT_TIME_MEMBER (this);
  printf (" %-12.12s", ctime (&tm) + 4);

  if (include_idle)
    {
      if (last_change)
	printf (" %s", idle_string (last_change));
      else
	printf ("   .  ");
    }
# ifdef HAVE_UT_HOST
  if (this->ut_host[0])
    {
      extern char *canon_host ();
      char ut_host[sizeof (this->ut_host) + 1];
      char *host = 0, *display = 0;

      /* Copy the host name into UT_HOST, and ensure it's nul terminated. */
      strncpy (ut_host, this->ut_host, (int) sizeof (this->ut_host));
      ut_host[sizeof (this->ut_host)] = '\0';

      /* Look for an X display.  */
      display = strrchr (ut_host, ':');
      if (display)
	*display++ = '\0';

      if (*ut_host)
	/* See if we can canonicalize it.  */
	host = canon_host (ut_host);
      if (! host)
	host = ut_host;

      if (display)
	printf (" (%s:%s)", host, display);
      else
	printf (" (%s)", host);
    }
# endif

  putchar ('\n');
}

/* Print the username of each valid entry and the number of valid entries
   in `utmp_contents', which should have N elements. */

static void
list_entries_who (int n)
{
  register STRUCT_UTMP *this = utmp_contents;
  int entries;

  entries = 0;
  while (n--)
    {
      if (this->ut_name[0]
# ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
# endif
	 )
	{
	  char *trimmed_name;

	  trimmed_name = extract_trimmed_name (this);

	  printf ("%s ", trimmed_name);
	  free (trimmed_name);
	  entries++;
	}
      this++;
    }
  printf (_("\n# users=%u\n"), entries);
}

#endif /* WHO */

#ifdef USERS

static int
userid_compare (const void *v_a, const void *v_b)
{
  char **a = (char **) v_a;
  char **b = (char **) v_b;
  return strcmp (*a, *b);
}

static void
list_entries_users (int n)
{
  register STRUCT_UTMP *this = utmp_contents;
  char **u;
  int i;
  int n_entries;

  n_entries = 0;
  u = (char **) xmalloc (n * sizeof (u[0]));
  for (i = 0; i < n; i++)
    {
      if (this->ut_name[0]
# ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
# endif
	 )
	{
	  char *trimmed_name;

	  trimmed_name = extract_trimmed_name (this);

	  u[n_entries] = trimmed_name;
	  ++n_entries;
	}
      this++;
    }

  qsort (u, n_entries, sizeof (u[0]), userid_compare);

  for (i = 0; i < n_entries; i++)
    {
      int c;
      fputs (u[i], stdout);
      c = (i < n_entries-1 ? ' ' : '\n');
      putchar (c);
    }

  for (i = 0; i < n_entries; i++)
    free (u[i]);
  free (u);
}

#endif /* USERS */

#ifdef WHO

static void
print_heading (void)
{
  printf ("%-8s ", _("USER"));
  if (include_mesg)
    printf (_("MESG "));
  printf ("%-8s ", _("LINE"));
  printf (_("LOGIN-TIME   "));
  if (include_idle)
    printf (_("IDLE  "));
  printf (_("FROM\n"));
}

/* Display `utmp_contents', which should have N entries. */

static void
scan_entries (int n)
{
  register STRUCT_UTMP *this = utmp_contents;

  if (include_heading)
    print_heading ();

  while (n--)
    {
      if (this->ut_name[0]
# ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
# endif
	 )
	print_entry (this);
      this++;
    }
}

#endif /* WHO */

/* Read the utmp file FILENAME into UTMP_CONTENTS and return the
   number of entries it contains. */

static int
read_utmp (char *filename)
{
  FILE *utmp;
  struct stat file_stats;
  size_t n_read;
  size_t size;

  utmp = fopen (filename, "r");
  if (utmp == NULL)
    error (1, errno, "%s", filename);

  fstat (fileno (utmp), &file_stats);
  size = file_stats.st_size;
  if (size > 0)
    utmp_contents = (STRUCT_UTMP *) xmalloc (size);
  else
    {
      fclose (utmp);
      return 0;
    }

  /* Use < instead of != in case the utmp just grew.  */
  n_read = fread (utmp_contents, 1, size, utmp);
  if (ferror (utmp) || fclose (utmp) == EOF
      || n_read < size)
    error (1, errno, "%s", filename);

  return size / sizeof (STRUCT_UTMP);
}

/* Display a list of who is on the system, according to utmp file FILENAME. */

static void
who (char *filename)
{
  int users;

  users = read_utmp (filename);
#ifdef WHO
  if (short_list)
    list_entries_who (users);
  else
    scan_entries (users);
#else
# ifdef USERS
  list_entries_users (users);
# else
#  ifdef UPTIME
  print_uptime (users);
#  endif /* UPTIME */
# endif /* USERS */
#endif /* WHO */
}

#ifdef WHO

/* Search `utmp_contents', which should have N entries, for
   an entry with a `ut_line' field identical to LINE.
   Return the first matching entry found, or NULL if there
   is no matching entry. */

static STRUCT_UTMP *
search_entries (int n, char *line)
{
  register STRUCT_UTMP *this = utmp_contents;

  while (n--)
    {
      if (this->ut_name[0]
# ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
# endif
	  && !strncmp (line, this->ut_line, sizeof (this->ut_line)))
	return this;
      this++;
    }
  return NULL;
}

/* Display the entry in utmp file FILENAME for this tty on standard input,
   or nothing if there is no entry for it. */

static void
who_am_i (char *filename)
{
  register STRUCT_UTMP *utmp_entry;
  char hostname[MAXHOSTNAMELEN + 1];
  char *tty;

  if (gethostname (hostname, MAXHOSTNAMELEN + 1))
    *hostname = 0;

  if (include_heading)
    {
      printf ("%*s ", (int) strlen (hostname), " ");
      print_heading ();
    }

  tty = ttyname (0);
  if (tty == NULL)
    return;
  tty += 5;			/* Remove "/dev/".  */

  utmp_entry = search_entries (read_utmp (filename), tty);
  if (utmp_entry == NULL)
    return;

  printf ("%s!", hostname);
  print_entry (utmp_entry);
}

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [ FILE | ARG1 ARG2 ]\n"), program_name);
      printf (_("\
\n\
  -H, --heading     print line of column headings\n\
  -i, -u, --idle    add user idle time as HOURS:MINUTES, . or old\n\
  -m                only hostname and user associated with stdin\n\
  -q, --count       all login names and number of users logged on\n\
  -s                (ignored)\n\
  -T, -w, --mesg    add user's message status as +, - or ?\n\
      --message     same as -T\n\
      --writable    same as -T\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n\
\n\
If FILE is not specified, use %s.  %s as FILE is common.\n\
If ARG1 ARG2 given, -m presumed: `am i' or `mom likes' are usual.\n\
"), UTMP_FILE, WTMP_FILE);
      puts (_("\nReport bugs to sh-utils-bugs@gnu.ai.mit.edu"));
    }
  exit (status);
}
#endif /* WHO */

#if defined(USERS) || defined(UPTIME)
static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [ FILE ]\n"), program_name);
      printf (_("\
Output who is currently logged in according to FILE.\n\
If FILE is not specified, use %s.  %s as FILE is common.\n\
\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n"),
	      UTMP_FILE, WTMP_FILE);
      puts (_("\nReport bugs to sh-utils-bugs@gnu.ai.mit.edu"));
    }
  exit (status);
}
#endif /* USERS || UPTIME */

int
main (int argc, char **argv)
{
  int optc, longind;
#ifdef WHO
  int my_line_only = 0;
#endif /* WHO */

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

#ifdef WHO
  while ((optc = getopt_long (argc, argv, "imqsuwHT", longopts, &longind))
#else
  while ((optc = getopt_long (argc, argv, "", longopts, &longind))
#endif /* WHO */
	 != EOF)
    {
      switch (optc)
	{
	case 0:
	  break;

#ifdef WHO
	case 'm':
	  my_line_only = 1;
	  break;

	case 'q':
	  short_list = 1;
	  break;

	case 's':
	  break;

	case 'i':
	case 'u':
	  include_idle = 1;
	  break;

	case 'H':
	  include_heading = 1;
	  break;

	case 'w':
	case 'T':
	  include_mesg = 1;
	  break;
#endif /* WHO */

	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s (%s) %s\n", COMMAND_NAME, GNU_PACKAGE, VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  switch (argc - optind)
    {
    case 0:			/* who */
#ifdef WHO
      if (my_line_only)
	who_am_i (UTMP_FILE);
      else
#endif /* WHO */
	who (UTMP_FILE);
      break;

    case 1:			/* who <utmp file> */
#ifdef WHO
      if (my_line_only)
	who_am_i (argv[optind]);
      else
#endif /* WHO */
	who (argv[optind]);
      break;

#ifdef WHO
    case 2:			/* who <blurf> <glop> */
      who_am_i (UTMP_FILE);
      break;
#endif /* WHO */

    default:			/* lose */
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  exit (0);
}
