/* mkdir -- make directories
   Copyright (C) 90, 95, 96, 97, 1998 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "modechange.h"
#include "makepath.h"
#include "closeout.h"
#include "error.h"

/* The name this program was run with. */
char *program_name;

/* If nonzero, ensure that all parents of the specified directory exist.  */
static int path_mode;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"parents", no_argument, &path_mode, 1},
  {"help", no_argument, &show_help, 1},
  {"verbose", no_argument, NULL, 2},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION] DIRECTORY...\n"), program_name);
      printf (_("\
Create the DIRECTORY(ies), if they do not already exist.\n\
\n\
  -m, --mode=MODE   set permission mode (as in chmod), not rwxrwxrwx - umask\n\
  -p, --parents     no error if existing, make parent directories as needed\n\
      --verbose     print a message for each created directory\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n\
"));
      puts (_("\nReport bugs to <fileutils-bugs@gnu.org>."));
      close_stdout ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  unsigned int newmode;
  unsigned int parent_mode;
  char *symbolic_mode = NULL;
  const char *verbose_fmt_string = NULL;
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  path_mode = 0;

  while ((optc = getopt_long (argc, argv, "pm:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:			/* Long option. */
	  break;
	case 'p':
	  path_mode = 1;
	  break;
	case 'm':
	  symbolic_mode = optarg;
	  break;
	case 2: /* --verbose  */
	  verbose_fmt_string = _("created directory `%s'");
	  break;
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("mkdir (%s) %s\n", GNU_PACKAGE, VERSION);
      close_stdout ();
      exit (0);
    }

  if (show_help)
    usage (0);

  if (optind == argc)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  newmode = 0777 & ~umask (0);
  parent_mode = newmode | 0300;	/* u+wx */
  if (symbolic_mode)
    {
      struct mode_change *change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, _("invalid mode `%s'"), symbolic_mode);
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, _("virtual memory exhausted"));
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      if (path_mode)
	{
	  errors |= make_path (argv[optind], newmode, parent_mode,
			       -1, -1, 1, verbose_fmt_string);
	}
      else if (mkdir (argv[optind], newmode))
	{
	  error (0, errno, _("cannot make directory `%s'"), argv[optind]);
	  errors = 1;
	}
      else if (verbose_fmt_string)
	{
	  error (0, 0, verbose_fmt_string, argv[optind]);
	}
    }

  exit (errors);
}
