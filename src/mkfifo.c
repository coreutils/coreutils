/* mkfifo -- make fifo's (named pipes)
   Copyright (C) 90, 91, 1995-2000 Free Software Foundation, Inc.

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

/* Options:
   -m, --mode=mode	Set the mode of created fifo's to MODE, which is
			symbolic as in chmod and uses the umask as a point of
			departure.

   David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "error.h"
#include "modechange.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mkfifo"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

#ifdef S_ISFIFO
void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] NAME...\n"), program_name);
      printf (_("\
Create named pipes (FIFOs) with the given NAMEs.\n\
\n\
  -m, --mode=MODE   set permission mode (as in chmod), not a=rw - umask\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}
#endif

int
main (int argc, char **argv)
{
  mode_t newmode;
  struct mode_change *change;
  const char *symbolic_mode;
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  symbolic_mode = NULL;

#ifndef S_ISFIFO
  error (4, 0, _("fifo files not supported"));
#else
  while ((optc = getopt_long (argc, argv, "m:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'm':
	  symbolic_mode = optarg;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (1);
	}
    }

  if (optind == argc)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  newmode = ((S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
	     & ~ umask (0));
  if (symbolic_mode)
    {
      change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, _("invalid mode"));
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, _("virtual memory exhausted"));
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      if (mkfifo (argv[optind], newmode))
	{
	  error (0, errno, _("cannot make fifo %s"), quote (argv[optind]));
	  errors = 1;
	}
    }

  exit (errors);
#endif
}
