/* GNU's who.
   Copyright (C) 1992-2003 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm; revised again by mstone */

/* Output format:
   name [state] line time [activity] [pid] [comment] [exit]
   state: -T
   name, line, time: not -q
   idle: -u
*/

#include <config.h>
#include <getopt.h>
#include <stdio.h>

#include <sys/types.h>
#include "system.h"

#include "readutmp.h"
#include "error.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "who"

#define AUTHORS N_ ("Joseph Arceneaux, David MacKenzie, and Michael Stone")

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#ifndef S_IWGRP
# define S_IWGRP 020
#endif

#ifndef USER_PROCESS
# define USER_PROCESS INT_MAX
#endif

#ifndef RUN_LVL
# define RUN_LVL INT_MAX
#endif

#ifndef INIT_PROCESS
# define INIT_PROCESS INT_MAX
#endif

#ifndef LOGIN_PROCESS
# define LOGIN_PROCESS INT_MAX
#endif

#ifndef DEAD_PROCESS
# define DEAD_PROCESS INT_MAX
#endif

#ifndef BOOT_TIME
# define BOOT_TIME 0
#endif

#ifndef NEW_TIME
# define NEW_TIME 0
#endif

#define IDLESTR_LEN 6

#if HAVE_STRUCT_XTMP_UT_PID
# define UT_PID(U) ((U)->ut_pid)
# define PIDSTR_DECL_AND_INIT(Var, Utmp_ent) \
  char Var[INT_STRLEN_BOUND (Utmp_ent->ut_pid) + 1]; \
  sprintf (Var, "%d", (int) (Utmp_ent->ut_pid))
#else
# define UT_PID(U) 0
# define PIDSTR_DECL_AND_INIT(Var, Utmp_ent) \
  const char *Var = ""
#endif

#if HAVE_STRUCT_XTMP_UT_ID
# define UT_ID(U) ((U)->ut_id)
#else
  /* Of course, sizeof "whatever" is the size of a pointer (often 4),
     but that's ok, since the actual string has a length of only 2.  */
# define UT_ID(U) "??"
#endif

#define UT_TYPE_UNDEF 255

#if HAVE_STRUCT_XTMP_UT_TYPE
# define UT_TYPE(U) ((U)->ut_type)
#else
# define UT_TYPE(U) UT_TYPE_UNDEF
#endif

#define IS_USER_PROCESS(U)			\
  (UT_USER (utmp_buf)[0]			\
   && (UT_TYPE (utmp_buf) == USER_PROCESS	\
       || (UT_TYPE (utmp_buf) == UT_TYPE_UNDEF	\
	   && UT_TIME_MEMBER (utmp_buf) != 0)))

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

/* If nonzero, display only name, line, and time fields */
static int short_output;

/* If nonzero, display the hours:minutes since each user has touched
   the keyboard, or "." if within the last minute, or "old" if
   not within the last day. */
static int include_idle;

/* If nonzero, display a line at the top describing each field. */
static int include_heading;

/* If nonzero, display a `+' for each user if mesg y, a `-' if mesg n,
   or a `?' if their tty cannot be statted. */
static int include_mesg;

/* If nonzero, display process termination & exit status */
static int include_exit;

/* If nonzero, display the last boot time */
static int need_boottime;

/* If nonzero, display dead processes */
static int need_deadprocs;

/* If nonzero, display processes waiting for user login */
static int need_login;

/* If nonzero, display processes started by init */
static int need_initspawn;

/* If nonzero, display the last clock change */
static int need_clockchange;

/* If nonzero, display the current runlevel */
static int need_runlevel;

/* If nonzero, display user processes */
static int need_users;

/* If nonzero, display info only for the controlling tty */
static int my_line_only;

/* for long options with no corresponding short option, use enum */
enum
{
  LOOKUP_OPTION = CHAR_MAX + 1,
  LOGIN_OPTION
};

static struct option const longopts[] = {
  {"all", no_argument, NULL, 'a'},
  {"boot", no_argument, NULL, 'b'},
  {"count", no_argument, NULL, 'q'},
  {"dead", no_argument, NULL, 'd'},
  {"heading", no_argument, NULL, 'H'},
  {"idle", no_argument, NULL, 'i'},
  {"login", no_argument, NULL, LOGIN_OPTION},
  {"lookup", no_argument, NULL, LOOKUP_OPTION},
  {"message", no_argument, NULL, 'T'},
  {"mesg", no_argument, NULL, 'T'},
  {"process", no_argument, NULL, 'p'},
  {"runlevel", no_argument, NULL, 'r'},
  {"short", no_argument, NULL, 's'},
  {"time", no_argument, NULL, 't'},
  {"users", no_argument, NULL, 'u'},
  {"writable", no_argument, NULL, 'T'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return a string representing the time between WHEN and the time
   that this function is first run.
   FIXME: locale? */
static const char *
idle_string (time_t when)
{
  static time_t now = 0;
  static char idle_hhmm[IDLESTR_LEN];
  time_t seconds_idle;

  if (now == 0)
    time (&now);

  seconds_idle = now - when;
  if (seconds_idle < 60)	/* One minute. */
    return "  .  ";
  if (seconds_idle < (24 * 60 * 60))	/* One day. */
    {
      sprintf (idle_hhmm, "%02d:%02d",
	       (int) (seconds_idle / (60 * 60)),
	       (int) ((seconds_idle % (60 * 60)) / 60));
      return (const char *) idle_hhmm;
    }
  return _(" old ");
}

/* Return a standard time string, "mon dd hh:mm"
   FIXME: handle localization */
static const char *
time_string (const STRUCT_UTMP *utmp_ent)
{
  /* Don't take the address of UT_TIME_MEMBER directly.
     Ulrich Drepper wrote:
     ``... GNU libc (and perhaps other libcs as well) have extended
     utmp file formats which do not use a simple time_t ut_time field.
     In glibc, ut_time is a macro which selects for backward compatibility
     the tv_sec member of a struct timeval value.''  */
  time_t tm = UT_TIME_MEMBER (utmp_ent);

  char *ptr = ctime (&tm) + 4;
  ptr[12] = '\0';
  return ptr;
}

/* Print formatted output line. Uses mostly arbitrary field sizes, probably
   will need tweaking if any of the localization stuff is done, or for 64 bit
   pids, etc. */
static void
print_line (const char *user, const char state, const char *line,
	    const char *time_str, const char *idle, const char *pid,
	    const char *comment, const char *exitstr)
{
  printf ("%-8.8s", user ? user : "   .");
  if (include_mesg)
    printf (" %c", state);
  printf (" %-12s", line);
  printf (" %-12s", time_str);
  if (include_idle && !short_output)
    printf (" %-6s", idle);
  if (!short_output)
    printf (" %10s", pid);
  /* FIXME: it's not really clear whether the following should be in short_output.
     a strict reading of SUSv2 would suggest not, but I haven't seen any
     implementations that actually work that way... */
  printf (" %-8s", comment);
  if (include_exit && exitstr && *exitstr)
    printf (" %-12s", exitstr);
  putchar ('\n');
}

/* Send properly parsed USER_PROCESS info to print_line */
static void
print_user (const STRUCT_UTMP *utmp_ent)
{
  struct stat stats;
  time_t last_change;
  char mesg;
  char idlestr[IDLESTR_LEN];
  static char *hoststr;
  static size_t hostlen;

#define DEV_DIR_WITH_TRAILING_SLASH "/dev/"
#define DEV_DIR_LEN (sizeof (DEV_DIR_WITH_TRAILING_SLASH) - 1)

  char line[sizeof (utmp_ent->ut_line) + DEV_DIR_LEN + 1];
  PIDSTR_DECL_AND_INIT (pidstr, utmp_ent);

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
      strncpy (line + DEV_DIR_LEN, utmp_ent->ut_line,
	       sizeof (utmp_ent->ut_line));
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

  if (last_change)
    sprintf (idlestr, "%.6s", idle_string (last_change));
  else
    sprintf (idlestr, "  ?");

#if HAVE_UT_HOST
  if (utmp_ent->ut_host[0])
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

      if (*ut_host && do_lookup)
	{
	  /* See if we can canonicalize it.  */
	  host = canon_host (ut_host);
	}

      if (! host)
	host = ut_host;

      if (display)
	{
	  if (hostlen < strlen (host) + strlen (display) + 4)
	    {
	      hostlen = strlen (host) + strlen (display) + 4;
	      hoststr = (char *) realloc (hoststr, hostlen);
	    }
	  sprintf (hoststr, "(%s:%s)", host, display);
	}
      else
	{
	  if (hostlen < strlen (host) + 3)
	    {
	      hostlen = strlen (host) + 3;
	      hoststr = (char *) realloc (hoststr, hostlen);
	    }
	  sprintf (hoststr, "(%s)", host);
	}
    }
  else
    {
      if (hostlen < 1)
	{
	  hostlen = 1;
	  hoststr = (char *) realloc (hoststr, hostlen);
	}
      stpcpy (hoststr, "");
    }
#endif

  print_line (UT_USER (utmp_ent), mesg, utmp_ent->ut_line,
	      time_string (utmp_ent), idlestr, pidstr,
	      hoststr ? hoststr : "", "");
}

static void
print_boottime (const STRUCT_UTMP *utmp_ent)
{
  print_line ("", ' ', "system boot", time_string (utmp_ent), "", "", "", "");
}

static char *
make_id_equals_comment (STRUCT_UTMP const *utmp_ent)
{
  char *comment = xmalloc (strlen (_("id=")) + sizeof UT_ID (utmp_ent) + 1);

  /* Cast field width argument to `int' to avoid warning from gcc.  */
  sprintf (comment, "%s%.*s", _("id="), (int) sizeof UT_ID (utmp_ent),
	   UT_ID (utmp_ent));
  return comment;
}

static void
print_deadprocs (const STRUCT_UTMP *utmp_ent)
{
  static char *exitstr;
  char *comment = make_id_equals_comment (utmp_ent);
  PIDSTR_DECL_AND_INIT (pidstr, utmp_ent);

  if (!exitstr)
    exitstr = xmalloc (strlen (_("term="))
		       + INT_STRLEN_BOUND (UT_EXIT_E_TERMINATION (utmp_ent)) + 1
		       + strlen (_("exit="))
		       + INT_STRLEN_BOUND (UT_EXIT_E_EXIT (utmp_ent))
		       + 1);
  sprintf (exitstr, "%s%d %s%d", _("term="), UT_EXIT_E_TERMINATION (utmp_ent),
	   _("exit="), UT_EXIT_E_EXIT (utmp_ent));

  /* FIXME: add idle time? */

  print_line ("", ' ', utmp_ent->ut_line,
	      time_string (utmp_ent), "", pidstr, comment, exitstr);
  free (comment);
}

static void
print_login (const STRUCT_UTMP *utmp_ent)
{
  char *comment = make_id_equals_comment (utmp_ent);
  PIDSTR_DECL_AND_INIT (pidstr, utmp_ent);

  /* FIXME: add idle time? */

  print_line ("LOGIN", ' ', utmp_ent->ut_line,
	      time_string (utmp_ent), "", pidstr, comment, "");
  free (comment);
}

static void
print_initspawn (const STRUCT_UTMP *utmp_ent)
{
  char *comment = make_id_equals_comment (utmp_ent);
  PIDSTR_DECL_AND_INIT (pidstr, utmp_ent);

  print_line ("", ' ', utmp_ent->ut_line,
	      time_string (utmp_ent), "", pidstr, comment, "");
  free (comment);
}

static void
print_clockchange (const STRUCT_UTMP *utmp_ent)
{
  /* FIXME: handle NEW_TIME & OLD_TIME both */
  print_line ("", ' ', _("clock change"),
	      time_string (utmp_ent), "", "", "", "");
}

static void
print_runlevel (const STRUCT_UTMP *utmp_ent)
{
  static char *runlevline, *comment;
  int last = UT_PID (utmp_ent) / 256;
  int curr = UT_PID (utmp_ent) % 256;

  if (!runlevline)
    runlevline = xmalloc (strlen (_("run-level")) + 3);
  sprintf (runlevline, "%s %c", _("run-level"), curr);

  if (!comment)
    comment = xmalloc (strlen (_("last=")) + 2);
  sprintf (comment, "%s%c", _("last="), (last == 'N') ? 'S' : last);

  print_line ("", ' ', runlevline, time_string (utmp_ent),
	      "", "", comment, "");

  return;
}

/* Print the username of each valid entry and the number of valid entries
   in UTMP_BUF, which should have N elements. */
static void
list_entries_who (int n, const STRUCT_UTMP *utmp_buf)
{
  int entries = 0;

  while (n--)
    {
      if (UT_USER (utmp_buf)[0] && UT_TYPE (utmp_buf) == USER_PROCESS)
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
  print_line (_("NAME"), ' ', _("LINE"), _("TIME"), _("IDLE"), _("PID"),
	      _("COMMENT"), _("EXIT"));
}

/* Display UTMP_BUF, which should have N entries. */
static void
scan_entries (int n, const STRUCT_UTMP *utmp_buf)
{
  char *ttyname_b IF_LINT ( = NULL);

  if (include_heading)
    print_heading ();

  if (my_line_only)
    {
      ttyname_b = ttyname (0);
      if (!ttyname_b)
	return;
      if (strncmp (ttyname_b, DEV_DIR_WITH_TRAILING_SLASH, DEV_DIR_LEN) == 0)
	ttyname_b += DEV_DIR_LEN;	/* Discard /dev/ prefix.  */
    }

  while (n--)
    {
      if (!my_line_only ||
	  strncmp (ttyname_b, utmp_buf->ut_line,
		   sizeof (utmp_buf->ut_line)) == 0)
	{
	  if (need_users && IS_USER_PROCESS (utmp_buf))
	    print_user (utmp_buf);
	  else if (need_runlevel && UT_TYPE (utmp_buf) == RUN_LVL)
	    print_runlevel (utmp_buf);
	  else if (need_boottime && UT_TYPE (utmp_buf) == BOOT_TIME)
	    print_boottime (utmp_buf);
	  /* I've never seen one of these, so I don't know what it should
	     look like :^)
	     FIXME: handle OLD_TIME also, perhaps show the delta? */
	  else if (need_clockchange && UT_TYPE (utmp_buf) == NEW_TIME)
	    print_clockchange (utmp_buf);
	  else if (need_initspawn && UT_TYPE (utmp_buf) == INIT_PROCESS)
	    print_initspawn (utmp_buf);
	  else if (need_login && UT_TYPE (utmp_buf) == LOGIN_PROCESS)
	    print_login (utmp_buf);
	  else if (need_deadprocs && UT_TYPE (utmp_buf) == DEAD_PROCESS)
	    print_deadprocs (utmp_buf);
	}

      utmp_buf++;
    }
}

/* Display a list of who is on the system, according to utmp file filename. */
static void
who (const char *filename)
{
  int n_users;
  STRUCT_UTMP *utmp_buf;
  int fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (EXIT_FAILURE, errno, "%s", filename);

  if (short_list)
    list_entries_who (n_users, utmp_buf);
  else
    scan_entries (n_users, utmp_buf);
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
      fputs (_("\
\n\
  -a, --all         same as -b -d --login -p -r -t -T -u\n\
  -b, --boot        time of last system boot\n\
  -d, --dead        print dead processes\n\
  -H, --heading     print line of column headings\n\
"), stdout);
      fputs (_("\
  -i, --idle        add idle time as HOURS:MINUTES, . or old\n\
                    (deprecated, use -u)\n\
      --login       print system login processes\n\
                    (equivalent to SUS -l)\n\
"), stdout);
      fputs (_("\
  -l, --lookup      attempt to canonicalize hostnames via DNS\n\
                    (-l is deprecated, use --lookup)\n\
  -m                only hostname and user associated with stdin\n\
  -p, --process     print active processes spawned by init\n\
"), stdout);
      fputs (_("\
  -q, --count       all login names and number of users logged on\n\
  -r, --runlevel    print current runlevel\n\
  -s, --short       print only name, line, and time (default)\n\
  -t, --time        print last system clock change\n\
"), stdout);
      fputs (_("\
  -T, -w, --mesg    add user's message status as +, - or ?\n\
  -u, --users       list users logged in\n\
      --message     same as -T\n\
      --writable    same as -T\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\
\n\
If FILE is not specified, use %s.  %s as FILE is common.\n\
If ARG1 ARG2 given, -m presumed: `am i' or `mom likes' are usual.\n\
"), UTMP_FILE, WTMP_FILE);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc, longind;
  int assumptions = 1;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "abdilmpqrstuwHT", longopts,
			      &longind)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'a':
	  need_boottime = 1;
	  need_deadprocs = 1;
	  need_login = 1;
	  need_initspawn = 1;
	  need_runlevel = 1;
	  need_clockchange = 1;
	  need_users = 1;
	  include_mesg = 1;
	  include_idle = 1;
	  include_exit = 1;
	  assumptions = 0;
	  break;

	case 'b':
	  need_boottime = 1;
	  assumptions = 0;
	  break;

	case 'd':
	  need_deadprocs = 1;
	  include_idle = 1;
	  include_exit = 1;
	  assumptions = 0;
	  break;

	case 'H':
	  include_heading = 1;
	  break;

	  /* FIXME: This should be -l in a future version */
	case LOGIN_OPTION:
	  need_login = 1;
	  include_idle = 1;
	  assumptions = 0;
	  break;

	case 'm':
	  my_line_only = 1;
	  break;

	case 'p':
	  need_initspawn = 1;
	  assumptions = 0;
	  break;

	case 'q':
	  short_list = 1;
	  break;

	case 'r':
	  need_runlevel = 1;
	  include_idle = 1;
	  assumptions = 0;
	  break;

	case 's':
	  short_output = 1;
	  break;

	case 't':
	  need_clockchange = 1;
	  assumptions = 0;
	  break;

	case 'T':
	case 'w':
	  include_mesg = 1;
	  break;

	case 'i':
	  error (0, 0,
		 _("Warning: -i will be removed in a future release; \
  use -u instead"));
	  /* Fall through.  */
	case 'u':
	  need_users = 1;
	  include_idle = 1;
	  assumptions = 0;
	  break;

	case 'l':
	  error (0, 0,
		 _("Warning: the meaning of '-l' will change in a future\
 release to conform to POSIX"));
	case LOOKUP_OPTION:
	  do_lookup = 1;
	  break;

	  case_GETOPT_HELP_CHAR;

	  case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (assumptions)
    {
      need_users = 1;
      short_output = 1;
    }

  if (include_exit)
    {
      short_output = 0;
    }

  switch (argc - optind)
    {
    case 0:			/* who */
      who (UTMP_FILE);
      break;

    case 1:			/* who <utmp file> */
      who (argv[optind]);
      break;

    case 2:			/* who <blurf> <glop> */
      my_line_only = 1;
      who (UTMP_FILE);
      break;

    default:			/* lose */
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
