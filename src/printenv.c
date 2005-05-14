/* printenv -- print all or part of environment
   Copyright (C) 1989-1997, 1999-2005 Free Software Foundation, Inc.

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

/* Usage: printenv [variable...]

   If no arguments are given, print the entire environment.
   If one or more variable names are given, print the value of
   each one that is set, and nothing for ones that are not set.

   Exit status:
   0 if all variables specified were found
   1 if not
   2 if some other error occurred

   David MacKenzie and Richard Mlynarik */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "long-options.h"

/* Exit status for syntax errors, etc.  */
enum { PRINTENV_FAILURE = 2 };

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "printenv"

#define AUTHORS "David MacKenzie", "Richard Mlynarik"

/* The name this program was run with. */
char *program_name;

extern char **environ;

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [VARIABLE]...\n\
  or:  %s OPTION\n\
If no environment VARIABLE specified, print them all.\n\
\n\
"),
	      program_name, program_name);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (USAGE_BUILTIN_WARNING, PROGRAM_NAME);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  char **env;
  char *ep, *ap;
  int i;
  bool ok;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  initialize_exit_failure (PRINTENV_FAILURE);
  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      usage, AUTHORS, (char const *) NULL);
  if (getopt_long (argc, argv, "+", NULL, NULL) != -1)
    usage (PRINTENV_FAILURE);

  if (optind >= argc)
    {
      for (env = environ; *env != NULL; ++env)
	puts (*env);
      ok = true;
    }
  else
    {
      int matches = 0;

      for (i = optind; i < argc; ++i)
	{
	  bool matched = false;

	  for (env = environ; *env; ++env)
	    {
	      ep = *env;
	      ap = argv[i];
	      while (*ep != '\0' && *ap != '\0' && *ep++ == *ap++)
		{
		  if (*ep == '=' && *ap == '\0')
		    {
		      puts (ep + 1);
		      matched = true;
		      break;
		    }
		}
	    }

	  matches += matched;
	}

      ok = (matches == argc - optind);
    }

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
