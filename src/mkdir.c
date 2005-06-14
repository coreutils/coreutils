/* mkdir -- make directories
   Copyright (C) 90, 1995-2002, 2004, 2005 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "dirname.h"
#include "error.h"
#include "mkdir-p.h"
#include "modechange.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mkdir"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

static struct option const longopts[] =
{
  {"mode", required_argument, NULL, 'm'},
  {"parents", no_argument, NULL, 'p'},
  {"verbose", no_argument, NULL, 'v'},
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
      printf (_("Usage: %s [OPTION] DIRECTORY...\n"), program_name);
      fputs (_("\
Create the DIRECTORY(ies), if they do not already exist.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -m, --mode=MODE   set permission mode (as in chmod), not rwxrwxrwx - umask\n\
  -p, --parents     no error if existing, make parent directories as needed\n\
  -v, --verbose     print a message for each created directory\n\
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
  mode_t newmode;
  mode_t parent_mode IF_LINT (= 0);
  const char *specified_mode = NULL;
  const char *verbose_fmt_string = NULL;
  bool create_parents = false;
  int exit_status = EXIT_SUCCESS;
  int optc;
  int cwd_errno = 0;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "pm:v", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'p':
	  create_parents = true;
	  break;
	case 'm':
	  specified_mode = optarg;
	  break;
	case 'v': /* --verbose  */
	  verbose_fmt_string = _("created directory %s");
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

  newmode = S_IRWXUGO;

  if (specified_mode || create_parents)
    {
      mode_t umask_value = umask (0);

      parent_mode = (S_IRWXUGO & ~umask_value) | (S_IWUSR | S_IXUSR);

      if (specified_mode)
	{
	  struct mode_change *change = mode_compile (specified_mode);
	  if (!change)
	    error (EXIT_FAILURE, 0, _("invalid mode %s"),
		   quote (specified_mode));
	  newmode = mode_adjust (S_IRWXUGO, change, umask_value);
	  free (change);
	}
      else
	umask (umask_value);
    }

  for (; optind < argc; ++optind)
    {
      char *dir = argv[optind];
      bool ok;

      if (create_parents)
	{
	  if (cwd_errno != 0 && IS_RELATIVE_FILE_NAME (dir))
	    {
	      error (0, cwd_errno, _("cannot return to working directory"));
	      ok = false;
	    }
	  else
	    ok = make_dir_parents (dir, newmode, parent_mode,
				   -1, -1, true, verbose_fmt_string,
				   &cwd_errno);
	}
      else
	{
	  ok = (mkdir (dir, newmode) == 0);

	  if (! ok)
	    error (0, errno, _("cannot create directory %s"), quote (dir));
	  else if (verbose_fmt_string)
	    error (0, 0, verbose_fmt_string, quote (dir));

	  /* mkdir(2) is required to honor only the file permission bits.
	     In particular, it needn't do anything about `special' bits,
	     so if any were set in newmode, apply them with chmod.
	     This extra step is necessary in some cases when the containing
	     directory has a default ACL.  */

	  /* Set the permissions only if this directory has just
	     been created.  */

	  if (ok && specified_mode
	      && chmod (dir, newmode) != 0)
	    {
	      error (0, errno, _("cannot set permissions of directory %s"),
		     quote (dir));
	      ok = false;
	    }
	}

      if (! ok)
	exit_status = EXIT_FAILURE;
    }

  exit (exit_status);
}
