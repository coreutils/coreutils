/* basename -- strip directory and suffix from filenames
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
#include "long-options.h"
#include "dirname.h"
#include "error.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "basename"

#define AUTHORS "FIXME unknown"

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
Usage: %s NAME [SUFFIX]\n\
  or:  %s OPTION\n\
"),
	      program_name, program_name);
      fputs (_("\
Print NAME with any leading directory components removed.\n\
If specified, also remove a trailing SUFFIX.\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Remove SUFFIX from the end of NAME if it is there, unless NAME
   consists entirely of SUFFIX. */

static void
remove_suffix (char *name, const char *suffix)
{
  char *np;
  const char *sp;

  np = name + strlen (name);
  sp = suffix + strlen (suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;
  if (np > name)
    *np = '\0';
}

int
main (int argc, char **argv)
{
  char *name;

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

  if (argc == 1 || argc > 3)
    {
      error (0, 0, (argc == 1 ? _("too few arguments")
		    : _("too many arguments")));
      usage (EXIT_FAILURE);
    }

  name = base_name (argv[1]);
  name[base_len (name)] = '\0';

  if (argc == 3)
    remove_suffix (name, argv[2]);

  puts (name);

  exit (EXIT_SUCCESS);
}
