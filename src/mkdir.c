/* mkdir -- make directories
   Copyright (C) 1990 Free Software Foundation, Inc.

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

/* Options:
   -p, --parent		Ensure that the given path(s) exist:
			Make any missing parent directories for each argument.
			Parent dirs default to umask modified by `u+wx'.
			Do not consider an argument directory that already
			exists to be an error.
   -m, --mode=mode	Set the mode of created directories to `mode', which is
			symbolic as in chmod and uses the umask as a point of
			departure.

   David MacKenzie <djm@ai.mit.edu>  */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "system.h"
#include "modechange.h"
#include "version.h"

int make_path ();
void error ();

static void usage ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, ensure that all parents of the specified directory exist.  */
static int path_mode;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"path", no_argument, &path_mode, 1},
  {"parents", no_argument, &path_mode, 1},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  unsigned int newmode;
  unsigned int parent_mode;
  char *symbolic_mode = NULL;
  int errors = 0;
  int optc;

  program_name = argv[0];
  path_mode = 0;

  while ((optc = getopt_long (argc, argv, "pm:", longopts, (int *) 0)) != EOF)
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
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (optind == argc)
    usage (1);

  newmode = 0777 & ~umask (0);
  parent_mode = newmode | 0300;	/* u+wx */
  if (symbolic_mode)
    {
      struct mode_change *change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, "invalid mode `%s'", symbolic_mode);
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, "virtual memory exhausted");
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      if (path_mode)
	errors |= make_path (argv[optind], newmode, parent_mode, -1, -1, NULL);
      else if (mkdir (argv[optind], newmode))
	{
	  error (0, errno, "cannot make directory `%s'", argv[optind]);
	  errors = 1;
	}
    }

  exit (errors);
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
      printf ("Usage: %s [OPTION] DIRECTORY...\n", program_name);
      printf ("\
\n\
  -p, --parents     no error if existing, make parent directories as needed\n\
  -m, --mode MODE   set permission mode (as in chmod), not 0777 - umask\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n");
    }
  exit (status);
}

