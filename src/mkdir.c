/* mkdir -- make directories
   Copyright (C) 90, 1995-2000 Free Software Foundation, Inc.

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
#include "error.h"
#include "makepath.h"
#include "modechange.h"
#include "quote.h"

#ifndef S_IRWXUGO
# define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mkdir"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

/* If nonzero, ensure that all parents of the specified directory exist.  */
static int create_parents;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"parents", no_argument, NULL, 'p'},
  {"verbose", no_argument, NULL, 'v'},
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
      printf (_("Usage: %s [OPTION] DIRECTORY...\n"), program_name);
      printf (_("\
Create the DIRECTORY(ies), if they do not already exist.\n\
\n\
  -m, --mode=MODE   set permission mode (as in chmod), not rwxrwxrwx - umask\n\
  -p, --parents     no error if existing, make parent directories as needed\n\
  -v, --verbose     print a message for each created directory\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  mode_t newmode;
  mode_t parent_mode;
  const char *symbolic_mode = NULL;
  const char *verbose_fmt_string = NULL;
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  create_parents = 0;

  while ((optc = getopt_long (argc, argv, "pm:v", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:			/* Long option. */
	  break;
	case 'p':
	  create_parents = 1;
	  break;
	case 'm':
	  symbolic_mode = optarg;
	  break;
	case 'v': /* --verbose  */
	  verbose_fmt_string = _("created directory %s");
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

  newmode = S_IRWXUGO & ~ umask (0);
  parent_mode = S_IWUSR | S_IXUSR | newmode;
  if (symbolic_mode)
    {
      struct mode_change *change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, _("invalid mode %s"), quote (symbolic_mode));
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, _("virtual memory exhausted"));
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      int fail = 0;
      if (create_parents)
	{
	  fail = make_path (argv[optind], newmode, parent_mode,
			    -1, -1, 1, verbose_fmt_string);
	}
      else
	{
	  fail = mkdir (argv[optind], newmode);
	  if (fail)
	    error (0, errno, _("cannot create directory %s"),
		   quote (argv[optind]));
	  else if (verbose_fmt_string)
	    error (0, 0, verbose_fmt_string, quote (argv[optind]));

	  /* mkdir(2) is required to honor only the file permission bits.
	     In particular, it needn't do anything about `special' bits,
	     so if any were set in newmode, apply them with chmod.  */
	  if (fail == 0 && (newmode & ~S_IRWXUGO))
	    {
	      fail = chmod (argv[optind], newmode);
	      if (fail)
		error (0, errno, _("cannot set permissions of directory %s"),
		       quote (argv[optind]));
	    }
	}

      errors |= fail;
    }

  exit (errors);
}
