/* dirname -- strip filename suffix from pathname
   Copyright (C) 90, 91, 92, 93, 94, 95, 1996 Free Software Foundation, Inc.

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

void strip_trailing_slashes ();

/* The name this program was run with. */
char *program_name;

static void
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
      printf (_("\
Print NAME with its trailing /component removed; if NAME contains no /'s,\n\
output `.' (meaning the current directory).\n\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
"));
      puts (_("\nReport bugs to sh-utils-bugs@gnu.ai.mit.edu"));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  register char *path;
  register char *slash;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  parse_long_options (argc, argv, "dirname", GNU_PACKAGE, VERSION, usage);

  if (argc != 2)
    {
      error (0, 0, argc < 2 ? _("too few arguments")
	     : _("too many arguments"));
      usage (1);
    }

  path = argv[1];
  strip_trailing_slashes (path);

  slash = strrchr (path, '/');
  if (slash == NULL)
    path = (char *) ".";
  else
    {
      /* Remove any trailing slashes and final element. */
      while (slash > path && *slash == '/')
	--slash;
      slash[1] = 0;
    }
  puts (path);

  exit (0);
}
