/* whoami -- print effective userid
   Copyright (C) 89,90, 1991-1997, 1999-2002 Free Software Foundation, Inc.

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

/* Equivalent to `id -un'. */
/* Written by Richard Mlynarik. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <getopt.h>

#include "system.h"
#include "long-options.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "whoami"

#define AUTHORS "Richard Mlynarik"

/* The name this program was run with. */
char *program_name;

static struct option const long_options[] =
{
  {0, 0, 0, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Print the user name associated with the current effective user id.\n\
Same as id -un.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  register struct passwd *pw;
  register uid_t uid;
  int c;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  while ((c = getopt_long (argc, argv, "", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind != argc)
    usage (EXIT_FAILURE);

  uid = geteuid ();
  pw = getpwuid (uid);
  if (pw)
    {
      puts (pw->pw_name);
      exit (EXIT_SUCCESS);
    }
  fprintf (stderr, _("%s: cannot find username for UID %u\n"),
	   program_name, (unsigned) uid);
  exit (EXIT_FAILURE);
}
