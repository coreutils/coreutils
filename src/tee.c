/* tee - read from standard input and write to standard output and files.
   Copyright (C) 85,1990-2002 Free Software Foundation, Inc.

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

/* Mike Parker, Richard M. Stallman, and David MacKenzie */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include "system.h"
#include "closeout.h"
#include "error.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "tee"

#define AUTHORS N_ ("Mike Parker, Richard M. Stallman, and David MacKenzie")

static int tee (int nfiles, const char **files);

/* If nonzero, append to output files rather than truncating them. */
static int append;

/* If nonzero, ignore interrupts. */
static int ignore_interrupts;

/* The name that this program was run with. */
char *program_name;

static struct option const long_options[] =
{
  {"append", no_argument, NULL, 'a'},
  {"ignore-interrupts", no_argument, NULL, 'i'},
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
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      fputs (_("\
Copy standard input to each FILE, and also to standard output.\n\
\n\
  -a, --append              append to the given FILEs, do not overwrite\n\
  -i, --ignore-interrupts   ignore interrupt signals\n\
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
  int errs;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  append = 0;
  ignore_interrupts = 0;

  while ((optc = getopt_long (argc, argv, "ai", long_options, NULL)) != -1)
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

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (ignore_interrupts)
    {
#ifdef _POSIX_SOURCE
      struct sigaction sigact;

      sigact.sa_handler = SIG_IGN;
      sigemptyset (&sigact.sa_mask);
      sigact.sa_flags = 0;
      sigaction (SIGINT, &sigact, NULL);
#else				/* !_POSIX_SOURCE */
      signal (SIGINT, SIG_IGN);
#endif				/* _POSIX_SOURCE */
    }

  /* Don't let us be killed if one of the output files is a pipe that
     doesn't consume all its input.  */
#ifdef _POSIX_SOURCE
  {
    struct sigaction sigact;

    sigact.sa_handler = SIG_IGN;
    sigemptyset (&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction (SIGPIPE, &sigact, NULL);
  }
#else
  signal (SIGPIPE, SIG_IGN);
#endif

  /* Do *not* warn if tee is given no file arguments.
     POSIX requires that it work when given no arguments.  */

  errs = tee (argc - optind, (const char **) &argv[optind]);
  if (close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (errs);
}

/* Copy the standard input into each of the NFILES files in FILES
   and into the standard output.
   Return 0 if successful, 1 if any errors occur. */

static int
tee (int nfiles, const char **files)
{
  FILE **descriptors;
  char buffer[BUFSIZ];
  int bytes_read, i;
  int ret = 0;
  const char *mode_string = (append ? "a" : "w");

  descriptors = (FILE **) xmalloc ((nfiles + 1) * sizeof (descriptors[0]));

  /* Move all the names `up' one in the argv array to make room for
     the entry for standard output.  This writes into argv[argc].  */
  for (i = nfiles; i >= 1; i--)
    files[i] = files[i - 1];

  SET_BINARY2 (0, 1);

  /* In the array of NFILES + 1 descriptors, make
     the first one correspond to standard output.   */
  descriptors[0] = stdout;
  files[0] = _("standard output");
  SETVBUF (stdout, NULL, _IONBF, 0);

  for (i = 1; i <= nfiles; i++)
    {
      descriptors[i] = fopen (files[i], mode_string);
      if (descriptors[i] == NULL)
	{
	  error (0, errno, "%s", files[i]);
	  ret = 1;
	}
      else
	{
	  SETVBUF (descriptors[i], NULL, _IONBF, 0);
	  SET_BINARY (fileno (descriptors[i]));
	}
    }

  while (1)
    {
      bytes_read = read (0, buffer, sizeof buffer);
#ifdef EINTR
      if (bytes_read < 0 && errno == EINTR)
        continue;
#endif
      if (bytes_read <= 0)
	break;

      /* Write to all NFILES + 1 descriptors.
	 Standard output is the first one.  */
      for (i = 0; i <= nfiles; i++)
	{
	  if (descriptors[i] != NULL)
	    fwrite (buffer, bytes_read, 1, descriptors[i]);
	}
    }

  if (bytes_read == -1)
    {
      error (0, errno, _("read error"));
      ret = 1;
    }

  /* Close the files, but not standard output.  */
  for (i = 1; i <= nfiles; i++)
    if (descriptors[i] != NULL
	&& (ferror (descriptors[i]) || fclose (descriptors[i]) == EOF))
      {
	error (0, errno, "%s", files[i]);
	ret = 1;
      }

  free (descriptors);

  return ret;
}
