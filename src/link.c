/* link utility for GNU.
   Copyright (C) 2001-2002 Free Software Foundation, Inc.

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

/* Written by Michael Stone */

/* Implementation overview:

   Simply call the system 'link' function */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "error.h"
#include "long-options.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "link"

#define AUTHORS "Michael Stone"

/* Name this program was run with.  */
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
Usage: %s FILE1 FILE2\n\
  or:  %s OPTION\n"), program_name, program_name);
      fputs (_("Call the link function to create a link named FILE2\
 to an existing FILE1.\n\n"),
	     stdout);
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

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);

  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle `--' here.  */
  if (1 < argc && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

  if (argc < 3)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  if (3 < argc)
    {
      error (0, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  if (link (argv[1], argv[2]) != 0)
    error (EXIT_FAILURE, errno, _("cannot create link %s to %s"),
	   quote_n (0, argv[2]), quote_n (1, argv[1]));

  exit (EXIT_SUCCESS);
}
