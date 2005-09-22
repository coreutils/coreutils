/* rmdir -- remove directories
   Copyright (C) 90, 91, 1995-2002, 2004, 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

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
#include "quotearg.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "rmdir"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

/* If true, remove empty parent directories.  */
static bool remove_empty_parents;

/* If true, don't treat failure to remove a nonempty directory
   as an error.  */
static bool ignore_fail_on_non_empty;

/* If true, output a diagnostic for every directory processed.  */
static bool verbose;

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

  {"path", no_argument, NULL, 'p'},  /* Deprecated.  */
  {"parents", no_argument, NULL, 'p'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return true if ERROR_NUMBER is one of the values associated
   with a failed rmdir due to non-empty target directory.  */

static bool
errno_rmdir_non_empty (int error_number)
{
  return (error_number == RMDIR_ERRNO_NOT_EMPTY);
}

/* Remove any empty parent directories of DIR.
   If DIR contains slash characters, at least one of them
   (beginning with the rightmost) is replaced with a NUL byte.
   Return true if successful.  */

static bool
remove_parents (char *dir)
{
  char *slash;
  bool ok = true;

  strip_trailing_slashes (dir);
  while (1)
    {
      slash = strrchr (dir, '/');
      if (slash == NULL)
	break;
      /* Remove any characters after the slash, skipping any extra
	 slashes in a row. */
      while (slash > dir && *slash == '/')
	--slash;
      slash[1] = 0;

      /* Give a diagnostic for each attempted removal if --verbose.  */
      if (verbose)
	error (0, 0, _("removing directory, %s"), dir);

      ok = (rmdir (dir) == 0);

      if (!ok)
	{
	  /* Stop quietly if --ignore-fail-on-non-empty. */
	  if (ignore_fail_on_non_empty
	      && errno_rmdir_non_empty (errno))
	    {
	      ok = true;
	    }
	  else
	    {
	      error (0, errno, "%s", quotearg_colon (dir));
	    }
	  break;
	}
    }
  return ok;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
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
  -p, --parents   Remove DIRECTORY and its ancestors.  E.g., `rmdir -p a/b/c' is\n\
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
  bool ok = true;
  int optc;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  remove_empty_parents = false;

  while ((optc = getopt_long (argc, argv, "pv", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'p':
	  remove_empty_parents = true;
	  break;
	case IGNORE_FAIL_ON_NON_EMPTY_OPTION:
	  ignore_fail_on_non_empty = true;
	  break;
	case 'v':
	  verbose = true;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    {
      error (0, 0, _("missing operand"));
      usage (EXIT_FAILURE);
    }

  for (; optind < argc; ++optind)
    {
      char *dir = argv[optind];

      /* Give a diagnostic for each attempted removal if --verbose.  */
      if (verbose)
	error (0, 0, _("removing directory, %s"), dir);

      if (rmdir (dir) != 0)
	{
	  if (ignore_fail_on_non_empty
	      && errno_rmdir_non_empty (errno))
	    continue;

	  error (0, errno, "%s", quotearg_colon (dir));
	  ok = false;
	}
      else if (remove_empty_parents)
	{
	  ok &= remove_parents (dir);
	}
    }

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
