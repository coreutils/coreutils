/* `rm' file deletion utility for GNU.
   Copyright (C) 88, 90, 91, 1994-2005 Free Software Foundation, Inc.

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
#include "dirname.h"
#include "error.h"
#include "lstat.h"
#include "quote.h"
#include "quotearg.h"
#include "remove.h"
#include "root-dev-ino.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "rm"

#define AUTHORS \
  "Paul Rubin", "David MacKenzie, Richard Stallman", "Jim Meyering"

/* Name this program was run with.  */
char *program_name;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  NO_PRESERVE_ROOT = CHAR_MAX + 1,
  PRESERVE_ROOT,
  PRESUME_INPUT_TTY_OPTION
};

static struct option const long_opts[] =
{
  {"directory", no_argument, NULL, 'd'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},

  {"no-preserve-root", no_argument, NULL, NO_PRESERVE_ROOT},
  {"preserve-root", no_argument, NULL, PRESERVE_ROOT},

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

/* Advise the user about invalid usages like "rm -foo" if the file
   "-foo" exists, assuming ARGC and ARGV are as with `main'.  */

static void
diagnose_leading_hyphen (int argc, char **argv)
{
  /* OPTIND is unreliable, so iterate through the arguments looking
     for a file name that looks like an option.  */
  int i;

  for (i = 1; i < argc; i++)
    {
      char const *arg = argv[i];
      struct stat st;

      if (arg[0] == '-' && arg[1] && lstat (arg, &st) == 0)
	{
	  fprintf (stderr,
		   _("Try `%s ./%s' to remove the file %s.\n"),
		   argv[0],
		   quotearg_n_style (1, shell_quoting_style, arg),
		   quote (arg));
	  break;
	}
    }
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      char *base = base_name (program_name);
      printf (_("Usage: %s [OPTION]... FILE...\n"), program_name);
      fputs (_("\
Remove (unlink) the FILE(s).\n\
\n\
  -d, --directory       unlink FILE, even if it is a non-empty directory\n\
                          (super-user only; this works only if your system\n\
                           supports `unlink' for nonempty directories)\n\
  -f, --force           ignore nonexistent files, never prompt\n\
  -i, --interactive     prompt before any removal\n\
"), stdout);
      fputs (_("\
      --no-preserve-root do not treat `/' specially (the default)\n\
      --preserve-root   fail to operate recursively on `/'\n\
  -r, -R, --recursive   remove directories and their contents recursively\n\
  -v, --verbose         explain what is being done\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
By default, rm does not remove directories.  Use the --recursive (-r or -R)\n\
option to remove each listed directory, too, along with all of its contents.\n\
"), stdout);
      printf (_("\
\n\
To remove a file whose name starts with a `-', for example `-foo',\n\
use one of these commands:\n\
  %s -- -foo\n\
\n\
  %s ./-foo\n\
"),
	      base, base);
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
  x->unlink_dirs = false;
  x->ignore_missing_files = false;
  x->interactive = false;
  x->recursive = false;
  x->root_dev_ino = NULL;
  x->stdin_tty = isatty (STDIN_FILENO);
  x->verbose = false;

  /* Since this program exits immediately after calling `rm', rm need not
     expend unnecessary effort to preserve the initial working directory.  */
  x->require_restore_cwd = false;
}

int
main (int argc, char **argv)
{
  bool preserve_root = false;
  struct rm_options x;
  int c;

  initialize_main (&argc, &argv);
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
	case 'd':
	  x.unlink_dirs = true;
	  break;

	case 'f':
	  x.interactive = false;
	  x.ignore_missing_files = true;
	  break;

	case 'i':
	  x.interactive = true;
	  x.ignore_missing_files = false;
	  break;

	case 'r':
	case 'R':
	  x.recursive = true;
	  break;

	case NO_PRESERVE_ROOT:
	  preserve_root = false;
	  break;

	case PRESERVE_ROOT:
	  preserve_root = true;
	  break;

	case PRESUME_INPUT_TTY_OPTION:
	  x.stdin_tty = true;
	  break;

	case 'v':
	  x.verbose = true;
	  break;

	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  diagnose_leading_hyphen (argc, argv);
	  usage (EXIT_FAILURE);
	}
    }

  if (argc <= optind)
    {
      if (x.ignore_missing_files)
	exit (EXIT_SUCCESS);
      else
	{
	  error (0, 0, _("missing operand"));
	  usage (EXIT_FAILURE);
	}
    }

  if (x.recursive & preserve_root)
    {
      static struct dev_ino dev_ino_buf;
      x.root_dev_ino = get_root_dev_ino (&dev_ino_buf);
      if (x.root_dev_ino == NULL)
	error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
	       quote ("/"));
    }

  {
    size_t n_files = argc - optind;
    char const *const *file = (char const *const *) argv + optind;

    enum RM_status status = rm (n_files, file, &x);
    assert (VALID_STATUS (status));
    exit (status == RM_ERROR ? EXIT_FAILURE : EXIT_SUCCESS);
  }
}
