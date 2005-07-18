/* Exit with a status code indicating success.
   Copyright (C) 1999-2003, 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include "system.h"

/* Act like "true" by default; false.c overrides this.  */
#ifndef EXIT_STATUS
# define EXIT_STATUS EXIT_SUCCESS
#endif

#if EXIT_STATUS == EXIT_SUCCESS
# define PROGRAM_NAME "true"
#else
# define PROGRAM_NAME "false"
#endif

#define AUTHORS "Jim Meyering"

/* The name this program was run with. */
char *program_name;

void
usage (int status)
{
  printf (_("\
Usage: %s [ignored command line arguments]\n\
  or:  %s OPTION\n\
"),
	  program_name, program_name);
  printf ("%s\n\n",
	  _(EXIT_STATUS == EXIT_SUCCESS
	    ? "Exit with a status code indicating success."
	    : "Exit with a status code indicating failure."));
  fputs (HELP_OPTION_DESCRIPTION, stdout);
  fputs (VERSION_OPTION_DESCRIPTION, stdout);
  printf (USAGE_BUILTIN_WARNING, PROGRAM_NAME);
  printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
  exit (status);
}

int
main (int argc, char **argv)
{
  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Recognize --help or --version only if it's the only command-line
     argument.  */
  if (argc == 2)
    {
      if (STREQ (argv[1], "--help"))
	usage (EXIT_STATUS);

      if (STREQ (argv[1], "--version"))
	version_etc (stdout, PROGRAM_NAME, GNU_PACKAGE, VERSION, AUTHORS,
		     (char *) NULL);
    }

  exit (EXIT_STATUS);
}
