/* printenv -- print all or part of environment
   Copyright (C) 1989-1997, 1999-2002 Free Software Foundation, Inc.

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

/* Usage: printenv [variable...]

   If no arguments are given, print the entire environment.
   If one or more variable names are given, print the value of
   each one that is set, and nothing for ones that are not set.

   Exit status:
   0 if all variables specified were found
   1 if not

   David MacKenzie and Richard Mlynarik */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "error.h"
#include "long-options.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "printenv"

#define AUTHORS N_ ("David MacKenzie and Richard Mlynarik")

/* The name this program was run with. */
char *program_name;

static struct option const long_options[] =
{
  {0, 0, 0, 0}
};

extern char **environ;

void
usage (int status)
{
  if (status != 0)
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
  int matches = 0;
  int c;
  int exit_status;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  close_stdout_set_status (2);
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

  if (optind == argc)
    {
      for (env = environ; *env != NULL; ++env)
	puts (*env);
      exit_status = 0;
    }
  else
    {
      for (i = optind; i < argc; ++i)
	{
	  for (env = environ; *env; ++env)
	    {
	      ep = *env;
	      ap = argv[i];
	      while (*ep != '\0' && *ap != '\0' && *ep++ == *ap++)
		{
		  if (*ep == '=' && *ap == '\0')
		    {
		      puts (ep + 1);
		      ++matches;
		      break;
		    }
		}
	    }
	}
      exit_status = (matches != argc - optind);
    }

  exit (exit_status);
}
