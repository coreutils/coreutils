/* rmdir -- remove directories
   Copyright (C) 90, 91, 1995-2002 Free Software Foundation, Inc.

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
#include "dirname.h"
#include "error.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "rmdir"

#define AUTHORS "David MacKenzie"

#ifndef EEXIST
# define EEXIST 0
#endif

#ifndef ENOTEMPTY
# define ENOTEMPTY 0
#endif

/* The name this program was run with. */
char *program_name;

/* If nonzero, remove empty parent directories. */
static int empty_paths;

/* If nonzero, don't treat failure to remove a nonempty directory
   as an error.  */
static int ignore_fail_on_non_empty;

/* If nonzero, output a diagnostic for every directory processed.  */
static int verbose;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  IGNORE_FAIL_ON_NON_EMPTY_OPTION = CHAR_MAX + 1
};

static struct option const longopts[] =
{
  /* Don't name this `--force' because it's not close enough in meaning
     to e.g. rm's -f option.  */
  {"ignore-fail-on-non-empty", no_argument, NULL,
   IGNORE_FAIL_ON_NON_EMPTY_OPTION},

  {"path", no_argument, NULL, 'p'},
  {"parents", no_argument, NULL, 'p'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return nonzero if ERROR_NUMBER is one of the values associated
   with a failed rmdir due to non-empty target directory.  */

static int
errno_rmdir_non_empty (int error_number)
{
  return (error_number == RMDIR_ERRNO_NOT_EMPTY);
}

/* Remove any empty parent directories of PATH.
   If PATH contains slash characters, at least one of them
   (beginning with the rightmost) is replaced with a NUL byte.  */

static int
remove_parents (char *path)
{
  char *slash;
  int fail = 0;

  strip_trailing_slashes (path);
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

      /* Give a diagnostic for each attempted removal if --verbose.  */
      if (verbose)
	error (0, 0, _("removing directory, %s"), path);

      fail = rmdir (path);

      if (fail)
	{
	  /* Stop quietly if --ignore-fail-on-non-empty. */
	  if (ignore_fail_on_non_empty
	      && errno_rmdir_non_empty (errno))
	    {
	      fail = 0;
	    }
	  else
	    {
	      error (0, errno, "%s", quote (path));
	    }
	  break;
	}
    }
  return fail;
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... DIRECTORY...\n"), program_name);
      fputs (_("\
Remove the DIRECTORY(ies), if they are empty.\n\
\n\
      --ignore-fail-on-non-empty\n\
                  ignore each failure that is solely because a directory\n\
                  is non-empty\n\
"), stdout);
      fputs (_("\
  -p, --parents   remove DIRECTORY, then try to remove each directory\n\
                  component of that path name.  E.g., `rmdir -p a/b/c' is\n\
                  similar to `rmdir a/b/c a/b a'.\n\
  -v, --verbose   output a diagnostic for every directory processed\n\
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
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  empty_paths = 0;

  while ((optc = getopt_long (argc, argv, "pv", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:			/* Long option. */
	  break;
	case 'p':
	  empty_paths = 1;
	  break;
	case IGNORE_FAIL_ON_NON_EMPTY_OPTION:
	  ignore_fail_on_non_empty = 1;
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

  if (optind == argc)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  for (; optind < argc; ++optind)
    {
      int fail;
      char *dir = argv[optind];

      /* Give a diagnostic for each attempted removal if --verbose.  */
      if (verbose)
	error (0, 0, _("removing directory, %s"), dir);

      fail = rmdir (dir);

      if (fail)
	{
	  if (ignore_fail_on_non_empty
	      && errno_rmdir_non_empty (errno))
	    continue;

	  error (0, errno, "%s", quote (dir));
	  errors = 1;
	}
      else if (empty_paths)
	{
	  errors += remove_parents (dir);
	}
    }

  exit (errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
