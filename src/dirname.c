/* dirname -- strip filename suffix from pathname
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

/* Written by David MacKenzie and Jim Meyering. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "long-options.h"
#include "error.h"
#include "dirname.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "dirname"

#define AUTHORS N_ ("David MacKenzie and Jim Meyering")

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
Usage: %s NAME\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      fputs (_("\
Print NAME with its trailing /component removed; if NAME contains no /'s,\n\
output `.' (meaning the current directory).\n\
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
  static char const dot = '.';
  char const *result;
  size_t len;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  parse_long_options (argc, argv, PROGRAM_NAME, GNU_PACKAGE, VERSION,
		      AUTHORS, usage);
  /* The above handles --help and --version.
     Since there is no other invocation of getopt, handle `--' here.  */
  if (argc > 1 && STREQ (argv[1], "--"))
    {
      --argc;
      ++argv;
    }

  if (argc != 2)
    {
      error (0, 0, argc < 2 ? _("too few arguments")
	     : _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  result = argv[1];
  len = dir_len (result);

  if (! len)
    {
      result = &dot;
      len = 1;
    }

  fwrite (result, 1, len, stdout);
  putchar ('\n');

  exit (EXIT_SUCCESS);
}
