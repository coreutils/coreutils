/* basename -- strip directory and suffix from filenames
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

/* Usage: basename name [suffix]
   NAME is a pathname; SUFFIX is a suffix to strip from it.

   basename /usr/foo/lossage/functions.l
   => functions.l
   basename /usr/foo/lossage/functions.l .l
   => functions
   basename functions.lisp p
   => functions.lis */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#include "system.h"
#include "version.h"
#include "long-options.h"

char *basename ();
void strip_trailing_slashes ();

static void remove_suffix ();

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
Usage: %s PATH [SUFFIX]\n\
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
  char *name;

  program_name = argv[0];

  parse_long_options (argc, argv, "basename", version_string, usage);

  if (argc == 1 || argc > 3)
    usage (1);

  strip_trailing_slashes (argv[1]);

  name = basename (argv[1]);

  if (argc == 3)
    remove_suffix (name, argv[2]);

  puts (name);

  exit (0);
}

/* Remove SUFFIX from the end of NAME if it is there, unless NAME
   consists entirely of SUFFIX. */

static void
remove_suffix (name, suffix)
     register char *name, *suffix;
{
  register char *np, *sp;

  np = name + strlen (name);
  sp = suffix + strlen (suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;
  if (np > name)
    *np = '\0';
}
