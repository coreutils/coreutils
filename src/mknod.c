/* mknod -- make special files
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

/* Usage: mknod [-m mode] [--mode=mode] path {bcu} major minor
		make a block or character device node
          mknod [-m mode] [--mode=mode] path p
		make a FIFO (named pipe)

   Options:
   -m, --mode=mode	Set the mode of created nodes to MODE, which is
			symbolic as in chmod and uses the umask as a point of
			departure.

   David MacKenzie <djm@ai.mit.edu>  */

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
static int flag_help;

/* If non-zero, print the version on standard error.  */
static int flag_version;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"help", no_argument, &flag_help, 1},
  {"version", no_argument, &flag_version, 1},
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
  int optc;

  program_name = argv[0];
  symbolic_mode = NULL;

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
	  usage ();
	}
    }

  if (flag_version)
    fprintf (stderr, "%s\n", version_string);

  if (flag_help)
    usage ();

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

  if (argc - optind != 2 && argc - optind != 4)
    usage ();

  /* Only check the first character, to allow mnemonic usage like
     `mknod /dev/rst0 character 18 0'. */

  switch (argv[optind + 1][0])
    {
    case 'b':			/* `block' or `buffered' */
#ifndef S_IFBLK
      error (4, 0, "block special files not supported");
#else
      if (argc - optind != 4)
	usage ();
      if (mknod (argv[optind], newmode | S_IFBLK,
		 makedev (atoi (argv[optind + 2]), atoi (argv[optind + 3]))))
	error (1, errno, "%s", argv[optind]);
#endif
      break;

    case 'c':			/* `character' */
    case 'u':			/* `unbuffered' */
#ifndef S_IFCHR
      error (4, 0, "character special files not supported");
#else
      if (argc - optind != 4)
	usage ();
      if (mknod (argv[optind], newmode | S_IFCHR,
		 makedev (atoi (argv[optind + 2]), atoi (argv[optind + 3]))))
	error (1, errno, "%s", argv[optind]);
#endif
      break;

    case 'p':			/* `pipe' */
#ifndef S_ISFIFO
      error (4, 0, "fifo files not supported");
#else
      if (argc - optind != 2)
	usage ();
      if (mkfifo (argv[optind], newmode))
	error (1, errno, "%s", argv[optind]);
#endif
      break;

    default:
      usage ();
    }

  exit (0);
}

static void
usage ()
{
  fprintf (stderr, "\
Usage: %s [options] path {bcu} major minor\n\
       %s [options] path p\n\
Options:\n\
       [-m mode] [--mode=mode] [--help] [--version]\n",
	   program_name, program_name);
  exit (1);
}
