/* GNU's users.
   Copyright (C) 92, 93, 94, 95, 96, 1997, 1998 Free Software Foundation, Inc.

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

#include <config.h>
#include <getopt.h>

#include "error.h"
#include "readutmp.h"
#include "system.h"

/* The name this program was run with. */
char *program_name;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static int
userid_compare (const void *v_a, const void *v_b)
{
  char **a = (char **) v_a;
  char **b = (char **) v_b;
  return strcmp (*a, *b);
}

static void
list_entries_users (int n, const STRUCT_UTMP *this)
{
  char **u;
  int i;
  int n_entries;

  n_entries = 0;
  u = (char **) xmalloc (n * sizeof (u[0]));
  for (i = 0; i < n; i++)
    {
      if (this->ut_name[0]
#ifdef USER_PROCESS
	  && this->ut_type == USER_PROCESS
#endif
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
      c = (i < n_entries - 1 ? ' ' : '\n');
      putchar (c);
    }

  for (i = 0; i < n_entries; i++)
    free (u[i]);
  free (u);
}

/* Display a list of users on the system, according to utmp file FILENAME. */

static void
users (const char *filename)
{
  int n_users;
  STRUCT_UTMP *utmp_buf;
  int fail = read_utmp (filename, &n_users, &utmp_buf);

  if (fail)
    error (1, errno, "%s", filename);

  list_entries_users (n_users, utmp_buf);
}

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
      puts (_("\nReport bugs to <bug-sh-utils@gnu.org>."));
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

  while ((optc = getopt_long (argc, argv, "", longopts, &longind)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("users (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  switch (argc - optind)
    {
    case 0:			/* users */
      users (UTMP_FILE);
      break;

    case 1:			/* users <utmp file> */
      users (argv[optind]);
      break;

    default:			/* lose */
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  exit (0);
}
