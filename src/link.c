/* `link` utility for GNU.
   Copyright (C) 2001 Free Software Foundation, Inc.

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

/* Written by Michael Stone */

/* Implementation overview:

   Simply calls the system 'link' function */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "error.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "link"

#define AUTHORS "Michael Stone"

/* Name this program was run with.  */
char *program_name;

static struct option const long_opts[] =
{
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
      printf (_("Usage: %s [OPTION]... FILE1 FILE2\n"), program_name);
      printf (_("\
Create a new directory entry FILE2 pointing to the same file as FILE1.\n\
\n\
      --help            display this help and exit\n\
      --version         output version information and exit\n\
\n\
"),
	      program_name, program_name);
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int fail = 0;
  int c;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((c = getopt_long (argc, argv, "", long_opts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:		/* Long option.  */
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (1);
	}
    }

  if (optind+2 > argc)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  if (optind+2 < argc)
    {
      error (0, 0, _("too many arguments"));
      usage (1);
    }

  if (link(argv[optind],argv[optind+1]) != 0)
    {
      fail = 1;
      error (0, errno, _("linking %s to %s"),
	     quote_n(0,argv[optind]), quote_n(1,argv[optind+1]));
    }

  exit (fail);
}
