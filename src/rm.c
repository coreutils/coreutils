/* `rm' file deletion utility for GNU.
   Copyright (C) 88, 90, 91, 1994-2003 Free Software Foundation, Inc.

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

/* Written by Paul Rubin, David MacKenzie, and Richard Stallman.
   Reworked to use chdir and avoid recursion by Jim Meyering.  */

/* Implementation overview:

   In the `usual' case, RM saves no state for directories it is processing.
   When a removal fails (either due to an error or to an interactive `no'
   reply), the failure is noted (see description of `ht' in remove.c's
   remove_cwd_entries function) so that when/if the containing directory
   is reopened, RM doesn't try to remove the entry again.

   RM may delete arbitrarily deep hierarchies -- even ones in which file
   names (from root to leaf) are longer than the system-imposed maximum.
   It does this by using chdir to change to each directory in turn before
   removing the entries in that directory.

   RM detects directory cycles lazily.  See lib/cycle-check.c.

   RM is careful to avoid forming full file names whenever possible.
   A full file name is formed only when it is about to be used -- e.g.
   in a diagnostic or in an interactive-mode prompt.

   RM minimizes the number of lstat system calls it makes.  On systems
   that have valid d_type data in directory entries, RM makes only one
   lstat call per command line argument -- regardless of the depth of
   the hierarchy.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "error.h"
#include "remove.h"
#include "save-cwd.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "rm"

#define AUTHORS \
  N_ ("Paul Rubin, David MacKenzie, Richard Stallman, and Jim Meyering")

/* Name this program was run with.  */
char *program_name;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  PRESUME_INPUT_TTY_OPTION = CHAR_MAX + 1
};

static struct option const long_opts[] =
{
  {"directory", no_argument, NULL, 'd'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},

  /* This is solely for testing.  Do not document.  */
  /* It is relatively difficult to ensure that there is a tty on stdin.
     Since rm acts differently depending on that, without this option,
     it'd be harder to test the parts of rm that depend on that setting.  */
  {"presume-input-tty", no_argument, NULL, PRESUME_INPUT_TTY_OPTION},

  {"recursive", no_argument, NULL, 'r'},
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
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Remove (unlink) the FILE(s).\n\
\n\
  -d, --directory       unlink FILE, even if it is a non-empty directory\n\
                          (super-user only)\n\
  -f, --force           ignore nonexistent files, never prompt\n\
  -i, --interactive     prompt before any removal\n\
  -r, -R, --recursive   remove the contents of directories recursively\n\
  -v, --verbose         explain what is being done\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\
\n\
To remove a file whose name starts with a `-', for example `-foo',\n\
use one of these commands:\n\
  %s -- -foo\n\
\n\
  %s ./-foo\n\
"),
	      program_name, program_name);
      fputs (_("\
\n\
Note that if you use rm to remove a file, it is usually possible to recover\n\
the contents of that file.  If you want more assurance that the contents are\n\
truly unrecoverable, consider using shred.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

static void
rm_option_init (struct rm_options *x)
{
  x->unlink_dirs = 0;
  x->ignore_missing_files = 0;
  x->interactive = 0;
  x->recursive = 0;
  x->stdin_tty = isatty (STDIN_FILENO);
  x->verbose = 0;
}

int
main (int argc, char **argv)
{
  struct rm_options x;
  int fail = 0;
  int c;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  rm_option_init (&x);

  while ((c = getopt_long (argc, argv, "dfirvR", long_opts, NULL)) != -1)
    {
      switch (c)
	{
	case 0:		/* Long option.  */
	  break;
	case 'd':
	  x.unlink_dirs = 1;
	  break;
	case 'f':
	  x.interactive = 0;
	  x.ignore_missing_files = 1;
	  break;
	case 'i':
	  x.interactive = 1;
	  x.ignore_missing_files = 0;
	  break;
	case 'r':
	case 'R':
	  x.recursive = 1;
	  break;
	case PRESUME_INPUT_TTY_OPTION:
	  x.stdin_tty = 1;
	  break;
	case 'v':
	  x.verbose = 1;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (optind == argc)
    {
      if (x.ignore_missing_files)
	exit (EXIT_SUCCESS);
      else
	{
	  error (0, 0, _("too few arguments"));
	  usage (EXIT_FAILURE);
	}
    }

  {
    size_t n_files = argc - optind;
    char const *const *file = (char const *const *) argv + optind;

    enum RM_status status = rm (n_files, file, &x);
    assert (VALID_STATUS (status));
    if (status == RM_ERROR)
      fail = 1;
  }

  exit (fail);
}
