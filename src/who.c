/* GNU's who.
   Copyright (C) 1992-1999 Free Software Foundation, Inc.

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
#include <getopt.h>
#include <stdio.h>

#include "error.h"
#include "readutmp.h"
#include "system.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "who"

#define AUTHORS "Joseph Arceneaux and David MacKenzie"

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#ifndef S_IWGRP
# define S_IWGRP 020
#endif

int gethostname ();
char *ttyname ();
char *canon_host ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, attempt to canonicalize hostnames via a DNS lookup. */
static int do_lookup;

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

static struct option const longopts[] =
{
  {"count", no_argument, NULL, 'q'},
  {"idle", no_argument, NULL, 'u'},
  {"heading", no_argument, NULL, 'H'},
  {"lookup", no_argument, NULL, 'l'},
  {"message", no_argument, NULL, 'T'},
  {"mesg", no_argument, NULL, 'T'},
  {"writable", no_argument, NULL, 'T'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

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
    return "  .  ";
  if (seconds_idle < (24 * 60 * 60)) /* One day. */
    {
      sprintf (idle_hhmm, "%02d:%02d",
	       (int) (seconds_idle / (60 * 60)),
	       (int) ((seconds_idle % (60 * 60)) / 60));
      return (const char *) idle_hhmm;
    }
  return _(" old ");
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
      mesg = (stats.st_mode & S_IWGRP) ? '+' : '-';
      last_change = stats.st_atime;
    }
  else
    {
      mesg = '?';
      last_change = 0;
    }

  printf ("%-8.*s", (int) sizeof (utmp_ent->ut_name), utmp_ent->ut_name);
  if (include_mesg)
    printf ("  %c  ", mesg);
  printf (" %-8.*s", (int) sizeof (utmp_ent->ut_line), utmp_ent->ut_line);

  /* Don't take the address of UT_TIME_MEMBER directly.
     Ulrich Drepper wrote:
     ``... GNU libc (and perhaps other libcs as well) have extended
     utmp file formats which do not use a simple time_t ut_time field.
     In glibc, ut_time is a macro which selects for backward compatibility
     the tv_sec member of a struct timeval value.''  */
  tm = UT_TIME_MEMBER (utmp_ent);
  printf (" %-12.12s", ctime (&tm) + 4);

  if (include_idle)
    {
      if (last_change)
	printf (" %s", idle_string (last_change));
      else
	printf ("   .  ");
    }
#if HAVE_UT_HOST
  if (utmp_ent->ut_host[0] && do_lookup)
    {
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
      if (! host)
	host = ut_host;

      if (display)
	printf (" (%s:%s)", host, display);
      else
	printf (" (%s)", host);
    }
#endif

  putchar ('\n');
}

/* Print the username of each valid entry and the number of valid entries
   in UTMP_BUF, which should have N elements. */

static void
list_entries_who (int n, const STRUCT_UTMP *utmp_buf)
{
  int entries;

  entries = 0;
  while (n--)
    {
      if (utmp_buf->ut_name[0]
#ifdef USER_PROCESS
	  && utmp_buf->ut_type == USER_PROCESS
#endif
	 )
	{
	  char *trimmed_name;

	  trimmed_name = extract_trimmed_name (utmp_buf);

	  printf ("%s ", trimmed_name);
	  free (trimmed_name);
	  entries++;
	}
      utmp_buf++;
    }
  printf (_("\n# users=%u\n"), entries);
}

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

/* Display UTMP_BUF, which should have N entries. */

static void
scan_entries (int n, const STRUCT_UTMP *utmp_buf)
{
  if (include_heading)
    print_heading ();

  while (n--)
    {
      if (utmp_buf->ut_name[0]
#ifdef USER_PROCESS
	  && utmp_buf->ut_type == USER_PROCESS
#endif
	 )
	print_entry (utmp_buf);
      utmp_buf++;
    }
}

/* Display a list of who is on the system, according to utmp file FILENAME. */

static void
who (const char *filename)
{
  int n_users;
  STRUCT_UTMP *utmp_buf;
  int fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (1, errno, "%s", filename);

  if (short_list)
    list_entries_who (n_users, utmp_buf);
  else
    scan_entries (n_users, utmp_buf);
}

/* Search UTMP_CONTENTS, which should have N entries, for
   an entry with a `ut_line' field identical to LINE.
   Return the first matching entry found, or NULL if there
   is no matching entry. */

static const STRUCT_UTMP *
search_entries (int n, const STRUCT_UTMP *utmp_buf, const char *line)
{
  while (n--)
    {
      if (utmp_buf->ut_name[0]
#ifdef USER_PROCESS
	  && utmp_buf->ut_type == USER_PROCESS
#endif
	  && !strncmp (line, utmp_buf->ut_line, sizeof (utmp_buf->ut_line)))
	return utmp_buf;
      utmp_buf++;
    }
  return NULL;
}

/* Display the entry in utmp file FILENAME for this tty on standard input,
   or nothing if there is no entry for it. */

static void
who_am_i (const char *filename)
{
  const STRUCT_UTMP *utmp_entry;
  STRUCT_UTMP *utmp_buf;
  char hostname[MAXHOSTNAMELEN + 1];
  char *tty;
  int fail;
  int n_users;

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

  fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (1, errno, "%s", filename);

  utmp_entry = search_entries (n_users, utmp_buf, tty);
  if (utmp_entry == NULL)
    return;

  printf ("%s!", hostname);
  print_entry (utmp_entry);
}

void
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
  -l, --lookup      attempt to canonicalize hostnames via DNS\n\
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
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc, longind;
  int my_line_only = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  while ((optc = getopt_long (argc, argv, "ilmqsuwHT", longopts, &longind))
	 != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'm':
	  my_line_only = 1;
	  break;

	case 'l':
	  do_lookup = 1;
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

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (1);
	}
    }

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
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  exit (0);
}
