/* mkfifo -- make fifo's (named pipes)
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

/* Options:
   -m, --mode=mode	Set the mode of created fifo's to MODE, which is
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

void error ();

static void usage ();

/* The name this program was run with. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  unsigned short newmode;
  struct mode_change *change;
  char *symbolic_mode;
  int errors = 0;
  int optc;

  program_name = argv[0];
  symbolic_mode = NULL;

#ifndef S_ISFIFO
  error (4, 0, "fifo files not supported");
#else
  while ((optc = getopt_long (argc, argv, "m:", longopts, (int *) 0)) != EOF)
    {
      switch (optc)
	{
	case 0:
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

  newmode = 0666 & ~umask (0);
  if (symbolic_mode)
    {
      change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, "invalid mode");
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, "virtual memory exhausted");
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      if (mkfifo (argv[optind], newmode))
	{
	  error (0, errno, "cannot make fifo `%s'", argv[optind]);
	  errors = 1;
	}
    }

  exit (errors);
#endif
}

#ifdef S_ISFIFO
static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION] PATH...\n", program_name);
      printf ("\
\n\
  -m, --mode MODE   set permission mode (as in chmod), not 0666 - umask\n\
      --help        display this help and exit\n\
      --version     output version information and exit\n");
    }
  exit (status);
}
#endif
