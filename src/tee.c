/* tee - read from standard input and write to standard output and files.
   Copyright (C) 1985, 1990, 1991 Free Software Foundation, Inc.

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

/* Mike Parker, Richard M. Stallman, and David MacKenzie */

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
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include "system.h"
#include "version.h"

char *xmalloc ();
void error ();
void xwrite ();

static int tee ();

/* If nonzero, append to output files rather than truncating them. */
static int append;

/* If nonzero, ignore interrupts. */
static int ignore_interrupts;

/* The name that this program was run with. */
char *program_name;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"append", no_argument, NULL, 'a'},
  {"help", no_argument, &show_help, 1},
  {"ignore-interrupts", no_argument, NULL, 'i'},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [FILE]...\n", program_name);
      printf ("\
\n\
  -a, --append              append to the given FILEs, do not overwrite\n\
  -i, --ignore-interrupts   ignore interrupt signals\n\
      --help                display this help and exit\n\
      --version             output version information and exit\n\
");
    }
  exit (status);
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  int errs;
  int optc;
	
  program_name = argv[0];
  append = 0;
  ignore_interrupts = 0;

  while ((optc = getopt_long (argc, argv, "ai", long_options, (int *) 0))
	 != EOF)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'a':
	  append = 1;
	  break;

	case 'i':
	  ignore_interrupts = 1;
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

  if (ignore_interrupts)
    {
#ifdef _POSIX_VERSION
      struct sigaction sigact;

      sigact.sa_handler = SIG_IGN;
      sigemptyset (&sigact.sa_mask);
      sigact.sa_flags = 0;
      sigaction (SIGINT, &sigact, NULL);
#else				/* !_POSIX_VERSION */
      signal (SIGINT, SIG_IGN);
#endif				/* _POSIX_VERSION */
    }

  errs = tee (argc - optind, &argv[optind]);
  if (close (0) != 0)
    error (1, errno, "standard input");
  if (close (1) != 0)
    error (1, errno, "standard output");
  exit (errs);
}

/* Copy the standard input into each of the NFILES files in FILES
   and into the standard output.
   Return 0 if successful, 1 if any errors occur. */

static int
tee (nfiles, files)
     int nfiles;
     char **files;
{
  int *descriptors;
  char buffer[BUFSIZ];
  register int bytes_read, i, ret = 0, mode;

  if (nfiles)
    descriptors = (int *) xmalloc (nfiles * sizeof (int));
  mode = O_WRONLY | O_CREAT;
  if (append)
    mode |= O_APPEND;
  else
    mode |= O_TRUNC;

  for (i = 0; i < nfiles; i++)
    {
      descriptors[i] = open (files[i], mode, 0666);
      if (descriptors[i] == -1)
	{
	  error (0, errno, "%s", files[i]);
	  ret = 1;
	}
    }

  while ((bytes_read = read (0, buffer, sizeof buffer)) > 0)
    {
      xwrite (1, buffer, bytes_read);
      for (i = 0; i < nfiles; i++)
	if (descriptors[i] != -1)
	  xwrite (descriptors[i], buffer, bytes_read);
    }
  if (bytes_read == -1)
    {
      error (0, errno, "read error");
      ret = 1;
    }

  for (i = 0; i < nfiles; i++)
    if (descriptors[i] != -1 && close (descriptors[i]) != 0)
      {
	error (0, errno, "%s", files[i]);
	ret = 1;
      }

  if (nfiles)
    free (descriptors);

  return ret;
}
