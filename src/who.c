/* GNU's who.
   Copyright (C) 1992 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>
#include <time.h>
#include <getopt.h>
#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif
#include "system.h"

#ifndef UTMP_FILE
#ifdef _PATH_UTMP		/* 4.4BSD.  */
#define UTMP_FILE _PATH_UTMP
#else				/* !_PATH_UTMP */
#define UTMP_FILE "/etc/utmp"
#endif				/* !_PATH_UTMP */
#endif				/* !UTMP_FILE */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define MESG_BIT 020		/* Group write bit. */

char *ttyname ();

char *idle_string ();
char *xmalloc ();
struct utmp *search_entries ();
void error ();
void list_entries ();
void print_entry ();
void print_heading ();
void scan_entries ();
void usage ();
void who ();
void who_am_i ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, display only a list of usernames and count of
   the users logged on.
   Ignored for `who am i'. */
int short_list;

/* If nonzero, display the hours:minutes since each user has touched
   the keyboard, or "." if within the last minute, or "old" if
   not within the last day. */
int include_idle;

/* If nonzero, display a line at the top describing each field. */
int include_heading;

/* If nonzero, display a `+' for each user if mesg y, a `-' if mesg n,
   or a `?' if their tty cannot be statted. */
int include_mesg;

struct option longopts[] =
{
  {"count", 0, NULL, 'q'},
  {"idle", 0, NULL, 'u'},
  {"heading", 0, NULL, 'H'},
  {"message", 0, NULL, 'T'},
  {"mesg", 0, NULL, 'T'},
  {"writable", 0, NULL, 'T'},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc, longind;
  int my_line_only = 0;

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "imqsuwHT", longopts, &longind))
	 != EOF)
    {
      switch (optc)
	{
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

	default:
	  usage ();
	}
    }

  if (chdir ("/dev"))
    error (1, errno, "cannot change directory to /dev");

  switch (argc - optind)
    {
    case 0:			/* who */
      if (my_line_only)
	who_am_i (UTMP_FILE);
      else
	who (UTMP_FILE);
      break;

    case 1:			/* who <utmp file> */
      if (my_line_only)
	who_am_i (argv[optind]);
      else
	who (argv[optind]);
      break;

    case 2:			/* who <blurf> <glop> */
      who_am_i (UTMP_FILE);
      break;

    default:			/* lose */
      usage ();
    }

  exit (0);
}

static struct utmp *utmp_contents;

/* Display a list of who is on the system, according to utmp file FILENAME. */

void
who (filename)
     char *filename;
{
  int users;

  users = read_utmp (filename);
  if (short_list)
    list_entries (users);
  else
    scan_entries (users);
}

/* Read the utmp file FILENAME into UTMP_CONTENTS and return the
   number of entries it contains. */

int
read_utmp (filename)
     char *filename;
{
  register int desc;
  struct stat file_stats;

  desc = open (filename, O_RDONLY, 0);
  if (desc < 0)
    error (1, errno, "%s", filename);

  fstat (desc, &file_stats);
  if (file_stats.st_size > 0)
    utmp_contents = (struct utmp *) xmalloc ((unsigned) file_stats.st_size);
  else
    {
      close (desc);
      return 0;
    }

  /* Use < instead of != in case the utmp just grew.  */
  if (read (desc, utmp_contents, file_stats.st_size) < file_stats.st_size)
    error (1, errno, "%s", filename);

  if (close (desc))
    error (1, errno, "%s", filename);

  return file_stats.st_size / sizeof (struct utmp);
}

/* Display a line of information about entry THIS. */

void
print_entry (this)
     struct utmp *this;
{
  struct stat stats;
  time_t last_change;
  char mesg;
  char line[sizeof (this->ut_line) + 1];

  strncpy (line, this->ut_line, sizeof (this->ut_line));
  line[sizeof (this->ut_line)] = 0;
  if (stat (line, &stats) == 0)
    {
      mesg = (stats.st_mode & MESG_BIT) ? '+' : '-';
      last_change = stats.st_atime;
    }
  else
    {
      mesg = '?';
      last_change = 0;
    }
  
  printf ("%-*.*s",
	  sizeof (this->ut_name), sizeof (this->ut_name),
	  this->ut_name);
  if (include_mesg)
    printf ("  %c  ", mesg);
  printf (" %-*.*s",
	  sizeof (this->ut_line), sizeof (this->ut_line),
	  this->ut_line);
  printf (" %-12.12s", ctime (&this->ut_time) + 4);
  if (include_idle)
    {
      if (last_change)
	printf (" %s", idle_string (last_change));
      else
	printf ("   .  ");
    }
#ifdef HAVE_UT_HOST
  if (this->ut_host[0])
    printf (" (%-.*s)", sizeof (this->ut_host), this->ut_host);
#endif

  putchar ('\n');
}

/* Print the username of each valid entry and the number of valid entries
   in `utmp_contents', which should have N elements. */

void
list_entries (n)
     int n;
{
  register struct utmp *this = utmp_contents;
  register int entries = 0;

  while (n--)
    {
      if (this->ut_name[0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
	 )
	{
	  printf ("%s ", this->ut_name);
	  entries++;
	}
      this++;
    }
  printf ("\n# users=%u\n", entries);
}

void
print_heading ()
{
  struct utmp *ut;

  printf ("%-*s ", sizeof (ut->ut_name), "USER");
  if (include_mesg)
    printf ("MESG ");
  printf ("%-*s ", sizeof (ut->ut_line), "LINE");
  printf ("LOGIN-TIME   ");
  if (include_idle)
    printf ("IDLE  ");
  printf ("FROM\n");
}

/* Display `utmp_contents', which should have N entries. */

void
scan_entries (n)
     int n;
{
  register struct utmp *this = utmp_contents;

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

struct utmp *
search_entries (n, line)
     int n;
     char *line;
{
  register struct utmp *this = utmp_contents;

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

void
who_am_i (filename)
     char *filename;
{
  register struct utmp *utmp_entry;
  char hostname[MAXHOSTNAMELEN + 1];
  char *tty;

  if (gethostname (hostname, MAXHOSTNAMELEN + 1))
    *hostname = 0;

  if (include_heading)
    {
      printf ("%*s ", strlen (hostname), " ");
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

char *
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
	       seconds_idle / (60 * 60),
	       (seconds_idle % (60 * 60)) / 60);
      return idle;
    }
  return " old ";
}

void
usage ()
{
  fprintf (stderr, "\
Usage: %s [-imqsuwHT] [--count] [--idle] [--heading] [--message] [--mesg]\n\
       [--writable] [file] [am i]\n",
	   program_name);
  exit (1);
}
