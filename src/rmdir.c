/* rmdir -- remove directories
   Copyright (C) 90, 91, 95, 96, 97, 1998 Free Software Foundation, Inc.

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
   -p, --parent		Remove any parent dirs that are explicitly mentioned
			in an argument, if they become empty after the
			argument file is removed.

   David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "closeout.h"
#include "error.h"

void strip_trailing_slashes ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, remove empty parent directories. */
static int empty_paths;

/* If nonzero, don't treat failure to remove a nonempty directory
   as an error.  */
static int ignore_fail_on_non_empty;

/* If nonzero, output a diagnostic for every directory processed.  */
static int verbose;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  /* Don't name this `--force' because it's not close enough in meaning
     to e.g. rm's -f option.  */
  {"ignore-fail-on-non-empty", no_argument, NULL, 13},

  {"path", no_argument, NULL, 'p'},
  {"parents", no_argument, NULL, 'p'},
  {"verbose", no_argument, NULL, 14},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

/* Remove any empty parent directories of PATH.
   If PATH contains slash characters, at least one of them
   (beginning with the rightmost) is replaced with a NUL byte.  */

static int
remove_parents (char *path)
{
  char *slash;
  int fail = 0;

  while (1)
    {
      slash = strrchr (path, '/');
      if (slash == NULL)
	break;
      /* Remove any characters after the slash, skipping any extra
	 slashes in a row. */
      while (slash > path && *slash == '/')
	--slash;
      slash[1] = 0;
      fail = rmdir (path);

      /* Give a diagnostic for each successful removal if --verbose.  */
      if (verbose && !fail)
	error (0, errno, _("removed directory, %s"), path);

      if (fail)
	{
	  /* Give a diagnostic and set fail if not --ignore.  */
	  if (!ignore_fail_on_non_empty || errno != ENOTEMPTY)
	    {
	      error (0, errno, "%s", path);
	      fail = 1;
	    }
	  break;
	}
    }
  return fail;
}

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... DIRECTORY...\n"), program_name);
      printf (_("\
Remove the DIRECTORY(ies), if they are empty.\n\
\n\
      --ignore-fail-on-non-empty\n\
                  ignore each failure that is solely because the\n\
                  directory is non-empty\n\
  -p, --parents   remove explicit parent directories if being emptied\n\
      --verbose   output a diagnostic for every directory processed\n\
      --help      display this help and exit\n\
      --version   output version information and exit\n\
"));
      puts (_("\nReport bugs to <fileutils-bugs@gnu.org>."));
      close_stdout ();
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  empty_paths = 0;

  while ((optc = getopt_long (argc, argv, "p", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:			/* Long option. */
	  break;
	case 'p':
	  empty_paths = 1;
	  break;
	case 13:
	  ignore_fail_on_non_empty = 1;
	  break;
	case 14:
	  verbose = 1;
	  break;
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("rmdir (%s) %s\n", GNU_PACKAGE, VERSION);
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

  for (; optind < argc; ++optind)
    {
      int fail;
      char *dir = argv[optind];

      /* Stripping slashes is harmless for rmdir;
	 if the arg is not a directory, it will fail with ENOTDIR.  */
      strip_trailing_slashes (dir);
      fail = rmdir (dir);

      /* Give a diagnostic for each successful removal if --verbose.  */
      if (verbose && !fail)
	error (0, errno, _("removed directory, %s"), dir);

      if (fail)
	{
	  if (!ignore_fail_on_non_empty || errno != ENOTEMPTY)
	    {
	      error (0, errno, "%s", dir);
	      errors = 1;
	    }
	}
      else if (empty_paths)
	errors += remove_parents (dir);
    }

  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
