/* logname -- print user's login name
   Copyright (C) 1990-1997, 1999-2002 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "long-options.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "logname"

#define AUTHORS "FIXME: unknown"

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
      printf (_("Usage: %s [OPTION]\n"), program_name);
      fputs (_("\
Print the name of the current user.\n\
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
  register char *cp;
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

  if (argc - optind != 0)
    usage (EXIT_FAILURE);

  /* POSIX requires using getlogin (or equivalent code).  */
  cp = getlogin ();
  if (cp)
    {
      puts (cp);
      exit (EXIT_SUCCESS);
    }
  /* POSIX prohibits using a fallback technique.  */
  fprintf (stderr, _("%s: no login name\n"), argv[0]);
  exit (EXIT_FAILURE);
}
