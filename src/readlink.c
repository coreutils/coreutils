/* readlink -- display value of a symbolic link.
   Copyright (C) 2002, 2003 Free Software Foundation, Inc.

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

/* Written by Dmitry V. Levin */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#include "system.h"
#include "canonicalize.h"
#include "error.h"
#include "xreadlink.h"
#include "long-options.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "readlink"

#define AUTHORS "Dmitry V. Levin"

/* Name this program was run with.  */
char *program_name;

  /* If nonzero, canonicalize file name. */
static int canonicalize;
  /* If nonzero, do not output the trailing newline. */
static int no_newline;
  /* If nonzero, report error messages. */
static int verbose;

static struct option const longopts[] =
{
  {"canonicalize", no_argument, 0, 'f'},
  {"no-newline", no_argument, 0, 'n'},
  {"quiet", no_argument, 0, 'q'},
  {"silent", no_argument, 0, 's'},
  {"verbose", no_argument, 0, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... FILE\n"), program_name);
      fputs (_("Display value of a symbolic link on standard output.\n\n"),
	     stdout);
      fputs (_("\
  -f, --canonicalize      canonicalize by following every symlink in every\n\
                          component of the given path recursively\n\
  -n, --no-newline        do not output the trailing newline\n\
  -q, --quiet,\n\
  -s, --silent            suppress most error messages\n\
  -v, --verbose           report error messages\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char *const argv[])
{
  const char *fname;
  char *value;
  int optc;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  if (argc < 1)
    error (EXIT_FAILURE, 0, _("too few arguments"));

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "fnqsv", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'f':
	  canonicalize = 1;
	  break;
	case 'n':
	  no_newline = 1;
	  break;
	case 'q':
	case 's':
	  verbose = 0;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  fname = argv[optind++];

  if (optind < argc)
    {
      error (EXIT_SUCCESS, 0, _("too many arguments"));
      usage (EXIT_FAILURE);
    }

  value = (canonicalize ? canonicalize_file_name : xreadlink) (fname);
  if (value)
    {
      printf ("%s%s", value, (no_newline ? "" : "\n"));
      free (value);
      return EXIT_SUCCESS;
    }

  if (verbose)
    error (EXIT_SUCCESS, errno, "%s", fname);

  return EXIT_FAILURE;
}
