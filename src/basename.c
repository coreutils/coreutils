/* basename -- strip directory and suffix from filenames
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "version.h"
#include "system.h"

char *basename ();
void strip_trailing_slashes ();

static void remove_suffix ();

/* The name this program was run with. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard error.  */
static int show_version;

static struct option const long_options[] =
{
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

static void
usage ()
{
  fprintf (stderr, "Usage: %s [{--help,--version}] name [suffix]\n",
	   program_name);
  exit (1);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  char *name;
  int c;

  program_name = argv[0];

  while ((c = getopt_long (argc, argv, "", long_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 0:
	  break;

	default:
	  usage ();
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage ();

  if (argc - optind == 0 || argc - optind > 2)
    usage ();

  strip_trailing_slashes (argv[optind]);

  name = basename (argv[optind]);

  if (argc == 3)
    remove_suffix (name, argv[optind + 1]);

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
