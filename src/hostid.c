/* print the hexadecimal identifier for the current host
   Copyright (C) 1997, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "system.h"
#include "closeout.h"
#include "long-options.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "hostid"

#define AUTHORS "Jim Meyering"

/* The name this program was run with, for error messages. */
char *program_name;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s\n\
  or:  %s OPTION\n\
Print the numeric identifier (in hexadecimal) for the current host.\n\
\n\
"),
	      program_name, program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  long int id;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  if (argc > 1)
    {
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  id = gethostid ();
  printf ("%lx\n", id);

  exit (EXIT_SUCCESS);
}
