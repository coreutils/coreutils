/* chroot -- run command or shell with special root directory
   Copyright (C) 1995 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>

#include "system.h"
#include "long-options.h"
#include "version.h"

void error ();

static void usage ();

/* The name this program was run with, for error messages. */
char *program_name;

int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;

  program_name = argv[0];

  parse_long_options (argc, argv, "chroot", version_string, usage);

  if (argc == 1)
    usage (1);

  if (chroot (argv[1]))
    error (1, errno, "cannot change root directory to %s", argv[1]);

  if (argc == 2)
    {
      /* No command.  Run an interactive shell.  */
      char *shell = getenv ("SHELL");
      if (shell == NULL)
	shell = "/bin/sh";
      argv[0] = shell;
      argv[1] = "-i";
    }
  else
    /* The following arguments give the command.  */
    argv += 2;

  /* Execute the given command.  */
  execvp (argv[0], argv);
  error (1, errno, "cannot execute %s", argv[0]);

  exit (1);
  return 1;
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION] NEWROOT [COMMAND...]\n", program_name);
      printf ("\
\n\
      --help       display this help and exit\n\
      --version    output version information and exit\n\
\n\
If no command is given, runs ``${SHELL} -i'' (default: /bin/sh).\n\
");
    }
  exit (status);
}
