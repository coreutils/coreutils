/* GNU's uptime/users/who.
   Copyright (C) 92, 93, 1994 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm */

/* Output format:
   name [state] line time [idle] host
   state: -T
   name, line, time: not -q
   idle: -u

   Options:
   -m		Same as 'who am i', for POSIX.
   -q		Only user names and # logged on; overrides all other options.
   -s		Name, line, time (default).
   -i, -u	Idle hours and minutes; '.' means active in last minute;
		'old' means idle for >24 hours.
   -H		Print column headings at top.
   -w, -T	-s plus mesg (+ or -, or ? if bad line). */

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
#include <sys/types.h>

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#define STRUCT_UTMP struct utmpx
#else
#include <utmp.h>
#define STRUCT_UTMP struct utmp
#endif

#include <time.h>
#include <getopt.h>
#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif

#include "system.h"
#include "version.h"

#if !defined (UTMP_FILE) && defined (_PATH_UTMP)	/* 4.4BSD.  */
#define UTMP_FILE _PATH_UTMP
#endif

#if defined (UTMPX_FILE)	/* Solaris, SysVr4 */
#undef  UTMP_FILE
#define UTMP_FILE UTMPX_FILE
#endif

#ifndef UTMP_FILE
#define UTMP_FILE "/etc/utmp"
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifndef S_IWGRP
#define S_IWGRP 020
#endif

#ifdef WHO
#define COMMAND_NAME "who"
#else
#ifdef USERS
#define COMMAND_NAME "users"
#else
 error You must define one of WHO and USERS.
#endif /* USERS */
#endif /* WHO */

char *xmalloc ();
void error ();
char *ttyname ();
char *gethostname ();

static int read_utmp ();
#ifdef WHO
static char *idle_string ();
static STRUCT_UTMP *search_entries ();
static void print_entry ();
static void print_heading ();
static void scan_entries ();
static void who_am_i ();
#endif /* WHO */
static void list_entries ();
static void usage ();
static void who ();

/* The name this program was run with. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
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

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc, longind;
#ifdef WHO
  int my_line_only = 0;
#endif /* WHO */

  program_name = argv[0];

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
	  error (0, 0, "too many arguments");
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s - %s\n", COMMAND_NAME, version_string);
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
      usage (1);
    }

  exit (0);
}

static STRUCT_UTMP *utmp_contents;

/* Display a list of who is on the system, according to utmp file FILENAME. */

static void
who (filename)
     char *filename;
{
  int users;

  users = read_utmp (filename);
#ifdef WHO
  if (short_list)
    list_entries (users);
  else
    scan_entries (users);
#else
#ifdef USERS
  list_entries (users);
#endif /* USERS */
#endif /* WHO */
}

/* Read the utmp file FILENAME into UTMP_CONTENTS and return the
   number of entries it contains. */

static int
read_utmp (filename)
     char *filename;
{
  FILE *utmp;
  struct stat file_stats;
  int n_read;

  utmp = fopen (filename, "r");
  if (utmp == NULL)
    error (1, errno, "%s", filename);

  fstat (fileno (utmp), &file_stats);
  if (file_stats.st_size > 0)
    utmp_contents = (STRUCT_UTMP *) xmalloc ((unsigned) file_stats.st_size);
  else
    {
      fclose (utmp);
      return 0;
    }

  /* Use < instead of != in case the utmp just grew.  */
  n_read = fread (utmp_contents, 1, file_stats.st_size, utmp);
  if (ferror (utmp) || fclose (utmp) == EOF
      || n_read < file_stats.st_size)
    error (1, errno, "%s", filename);

  return file_stats.st_size / sizeof (STRUCT_UTMP);
}

#ifdef WHO
/* Display a line of information about entry THIS. */

static void
print_entry (this)
     STRUCT_UTMP *this;
{
  struct stat stats;
  time_t last_change;
  char mesg;

#define DEV_DIR_WITH_TRAILING_SLASH "/dev/"
#define DEV_DIR_LEN (sizeof (DEV_DIR_WITH_TRAILING_SLASH) - 1)

  char line[sizeof (this->ut_line) + DEV_DIR_LEN + 1];

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
      strcpy(line, DEV_DIR_WITH_TRAILING_SLASH);
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

#ifdef HAVE_UTMPX_H
  printf (" %-12.12s", ctime (&this->ut_tv.tv_sec) + 4);
#else
  printf (" %-12.12s", ctime (&this->ut_time) + 4);
#endif

  if (include_idle)
    {
      if (last_change)
	printf (" %s", idle_string (last_change));
      else
	printf ("   .  ");
    }
#ifdef HAVE_UT_HOST
  if (this->ut_host[0])
    printf (" (%-.*s)", (int) sizeof (this->ut_host), this->ut_host);
#endif

  putchar ('\n');
}
#endif /* WHO */

#if defined (WHO) || defined (USERS)
/* Print the username of each valid entry and the number of valid entries
   in `utmp_contents', which should have N elements. */

static void
list_entries (n)
     int n;
{
  register STRUCT_UTMP *this = utmp_contents;
  register int entries = 0;

  while (n--)
    {
      if (this->ut_name[0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
	 )
	{
	  char trimmed_name[sizeof (this->ut_name) + 1];
	  int i;

	  strncpy (trimmed_name, this->ut_name, sizeof (this->ut_name));
	  trimmed_name[sizeof (this->ut_name)] = ' ';
	  for (i = 0; i <= sizeof (this->ut_name); i++)
	    {
	      if (trimmed_name[i] == ' ')
		break;
	    }
	  trimmed_name[i] = '\0';

	  printf ("%s ", trimmed_name);
	  entries++;
	}
      this++;
    }
#ifdef WHO
  printf ("\n# users=%u\n", entries);
#else
  putchar('\n');
#endif /* WHO */
}
#endif /* defined (WHO) || defined (USERS) */

#ifdef WHO

static void
print_heading ()
{
  printf ("%-8s ", "USER");
  if (include_mesg)
    printf ("MESG ");
  printf ("%-8s ", "LINE");
  printf ("LOGIN-TIME   ");
  if (include_idle)
    printf ("IDLE  ");
  printf ("FROM\n");
}

/* Display `utmp_contents', which should have N entries. */

static void
scan_entries (n)
     int n;
{
  register STRUCT_UTMP *this = utmp_contents;

  if (include_heading)
    print_heading ();

  while (n--)
    {
      if (this->ut_name[0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
	 )
	print_entry (this);
      this++;
    }
}

/* Search `utmp_contents', which should have N entries, for
   an entry with a `ut_line' field identical to LINE.
   Return the first matching entry found, or NULL if there
   is no matching entry. */

static STRUCT_UTMP *
search_entries (n, line)
     int n;
     char *line;
{
  register STRUCT_UTMP *this = utmp_contents;

  while (n--)
    {
      if (this->ut_name[0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
	  && !strncmp (line, this->ut_line, sizeof (this->ut_line)))
	return this;
      this++;
    }
  return NULL;
}

/* Display the entry in utmp file FILENAME for this tty on standard input,
   or nothing if there is no entry for it. */

static void
who_am_i (filename)
     char *filename;
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

/* Return a string representing the time between WHEN and the time
   that this function is first run. */

static char *
idle_string (when)
     time_t when;
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
      return idle;
    }
  return " old ";
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
      printf ("Usage: %s [OPTION]... [ FILE | ARG1 ARG2 ]\n", program_name);
      printf ("\
\n\
  -H, --heading     print line of column headings\n\
  -T, -w, --mesg    add user's message status as +, - or ?\n\
  -i, -u, --idle    add user idle time as HOURS:MINUTES, . or old\n\
  -m                only hostname and user associated with stdin\n\
  -q, --count       all login names and number of users logged on\n\
  -s                (ignored)\n\
      --help        display this help and exit\n\
      --message     same as -T\n\
      --version     output version information and exit\n\
      --writeable   same as -T\n\
\n\
If FILE not given, uses /etc/utmp.  /etc/wtmp as FILE is common.\n\
If ARG1 ARG2 given, -m presumed: `am i' or `mom likes' are usual.\n\
");
    }
  exit (status);
}
#endif /* WHO */

#if defined (USERS)
static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [ FILE ]\n", program_name);
      printf ("\
\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n\
\n\
If FILE not given, uses /etc/utmp.  /etc/wtmp as FILE is common.\n\
");
    }
  exit (status);
}
#endif /* USERS */
