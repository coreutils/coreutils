/* tty -- print the path of the terminal connected to standard input
   Copyright (C) 1990-2002 Free Software Foundation, Inc.

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

/* Displays "not a tty" if stdin is not a terminal.
   Displays nothing if -s option is given.
   Exit status 0 if stdin is a tty, 1 if not, 2 if usage error,
   3 if write error.

   Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tty"

#define AUTHORS "David MacKenzie"

/* The name under which this program was run. */
char *program_name;

/* If nonzero, return an exit status but produce no output. */
static int silent;

static struct option const longopts[] =
{
  {"silent", no_argument, NULL, 's'},
  {"quiet", no_argument, NULL, 's'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]...\n"), program_name);
      fputs (_("\
Print the file name of the terminal connected to standard input.\n\
\n\
  -s, --silent, --quiet   print nothing, only return an exit status\n\
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
  char *tty;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  close_stdout_set_status (3);
  atexit (close_stdout);

  silent = 0;

  while ((optc = getopt_long (argc, argv, "s", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 's':
	  silent = 1;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (2);
	}
    }

  if (optind != argc)
    error (0, 0, _("ignoring all arguments"));

  tty = ttyname (0);
  if (!silent)
    {
      if (tty)
	puts (tty);
      else
	puts (_("not a tty"));
    }

  exit (isatty (0) ? 0 : 1);
}
