/* mkdir -- make directories
   Copyright (C) 90, 1995-2002 Free Software Foundation, Inc.

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

/* David MacKenzie <djm@ai.mit.edu>  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "dirname.h"
#include "error.h"
#include "makepath.h"
#include "modechange.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mkdir"

#define AUTHORS "David MacKenzie"

/* The name this program was run with. */
char *program_name;

/* If nonzero, ensure that all parents of the specified directory exist.  */
static int create_parents;

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
  if (status != 0)
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
  mode_t parent_mode;
  const char *specified_mode = NULL;
  const char *verbose_fmt_string = NULL;
  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  create_parents = 0;

  while ((optc = getopt_long (argc, argv, "pm:v", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:			/* Long option. */
	  break;
	case 'p':
	  create_parents = 1;
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
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  newmode = S_IRWXUGO;
  {
    mode_t umask_value = umask (0);
    umask (umask_value);		/* Restore the old value. */
    parent_mode = (newmode & (~ umask_value)) | S_IWUSR | S_IXUSR;
  }

  if (specified_mode)
    {
      struct mode_change *change = mode_compile (specified_mode, 0);
      newmode &= ~ umask (0);
      if (change == MODE_INVALID)
	error (EXIT_FAILURE, 0, _("invalid mode %s"), quote (specified_mode));
      else if (change == MODE_MEMORY_EXHAUSTED)
	xalloc_die ();
      newmode = mode_adjust (newmode, change);
    }

  for (; optind < argc; ++optind)
    {
      int fail = 0;

      if (create_parents)
	{
	  char *dir = argv[optind];
	  fail = make_path (dir, newmode, parent_mode,
			    -1, -1, 1, verbose_fmt_string);
	}
      else
	{
	  const char *dir = argv[optind];
	  int dir_created;
	  fail = make_dir (dir, dir, newmode, &dir_created);
	  if (fail)
	    {
	      /* make_dir already gave a diagnostic.  */
	    }
	  else if (!create_parents && !dir_created)
	    {
	      /* make_dir `succeeds' when DIR already exists.
		 In that case, mkdir must fail, unless --parents (-p)
		 was specified.  */
	      error (0, EEXIST, _("cannot create directory %s"),
		     quote (dir));
	      fail = 1;
	    }
	  else if (verbose_fmt_string)
	    error (0, 0, verbose_fmt_string, quote (dir));

	  /* mkdir(2) is required to honor only the file permission bits.
	     In particular, it needn't do anything about `special' bits,
	     so if any were set in newmode, apply them with chmod.
	     This extra step is necessary in some cases when the containing
	     directory has a default ACL.  */

	  /* Set the permissions only if this directory has just
	     been created.  */

	  if (fail == 0 && specified_mode && dir_created)
	    {
	      fail = chmod (dir, newmode);
	      if (fail)
		error (0, errno, _("cannot set permissions of directory %s"),
		       quote (dir));
	    }
	}

      if (fail)
	errors = 1;
    }

  exit (errors);
}
