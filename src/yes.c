/* yes - output a string repeatedly until killed
   Copyright (C) 1991-1997, 1999-2002 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "error.h"
#include "system.h"
#include "long-options.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "yes"

#define AUTHORS "David MacKenzie"

/* How many iterations between ferror checks.  */
#define UNROLL 10000

/* The name this program was run with. */
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
Usage: %s [STRING]...\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);

      fputs (_("\
Repeatedly output a line with all specified STRING(s), or `y'.\n\
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
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* Don't recognize --help or --version if POSIXLY_CORRECT is set.  */
  if (getenv ("POSIXLY_CORRECT") == NULL)
    parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
			AUTHORS, usage);

  if (argc == 1)
    {
      while (1)
	{
	  int i;
	  for (i = 0; i < UNROLL; i++)
	    puts ("y");
	  if (ferror (stdout))
	    break;
	}
    }
  else
    {
      while (1)
	{
	  int i;
	  for (i = 0; i < UNROLL; i++)
	    {
	      int j;
	      for (j = 1; j < argc; j++)
		{
		  fputs (argv[j], stdout);
		  putchar (j == argc - 1 ? '\n' : ' ');
		}
	    }
	  if (ferror (stdout))
	    break;
	}
    }

  error (0, errno, "standard output");
  exit (EXIT_FAILURE);
}
