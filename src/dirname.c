/* dirname -- strip filename suffix from pathname
   Copyright (C) 90, 91, 92, 93, 1994 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by David MacKenzie and Jim Meyering. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "version.h"
#include "long-options.h"

void strip_trailing_slashes ();

/* The name this program was run with. */
char *program_name;

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("\
Usage: %s PATH\n\
  or:  %s OPTION\n\
",
	      program_name, program_name);
      printf ("\
\n\
  --help      display this help and exit\n\
  --version   output version information and exit\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  register char *path;
  register char *slash;

  program_name = argv[0];

  parse_long_options (argc, argv, "dirname", version_string, usage);

  if (argc != 2)
    usage (1);

  path = argv[1];
  strip_trailing_slashes (path);

  slash = rindex (path, '/');
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
