/* chmod -- change permission modes of files
   Copyright (C) 89, 90, 91, 1995-2004 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>

#include "system.h"
#include "dev-ino.h"
#include "dirname.h"
#include "error.h"
#include "filemode.h"
#include "modechange.h"
#include "quote.h"
#include "root-dev-ino.h"
#include "savedir.h"
#include "xfts.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "chmod"

#define AUTHORS "David MacKenzie", "Jim Meyering"

enum Change_status
{
  CH_SUCCEEDED,
  CH_FAILED,
  CH_NO_CHANGE_REQUESTED
};

enum Verbosity
{
  /* Print a message for each file that is processed.  */
  V_high,

  /* Print a message for each file whose attributes we change.  */
  V_changes_only,

  /* Do not be verbose.  This is the default. */
  V_off
};

/* The name the program was run with. */
char *program_name;

/* If nonzero, change the modes of directories recursively. */
static int recurse;

/* If nonzero, force silence (no error messages). */
static int force_silent;

/* Level of verbosity.  */
static enum Verbosity verbosity = V_off;

/* The argument to the --reference option.  Use the owner and group IDs
   of this file.  This file must exist.  */
static char *reference_file;

/* Pointer to the device and inode numbers of `/', when --recursive.
   Otherwise NULL.  */
static struct dev_ino *root_dev_ino;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  NO_PRESERVE_ROOT = CHAR_MAX + 1,
  PRESERVE_ROOT,
  REFERENCE_FILE_OPTION
};

static struct option const long_options[] =
{
  {"changes", no_argument, 0, 'c'},
  {"recursive", no_argument, 0, 'R'},
  {"no-preserve-root", no_argument, 0, NO_PRESERVE_ROOT},
  {"preserve-root", no_argument, 0, PRESERVE_ROOT},
  {"quiet", no_argument, 0, 'f'},
  {"reference", required_argument, 0, REFERENCE_FILE_OPTION},
  {"silent", no_argument, 0, 'f'},
  {"verbose", no_argument, 0, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0}
};

static int
mode_changed (const char *file, mode_t old_mode)
{
  struct stat new_stats;

  if (stat (file, &new_stats))
    {
      if (force_silent == 0)
	error (0, errno, _("getting new attributes of %s"), quote (file));
      return 0;
    }

  return old_mode != new_stats.st_mode;
}

/* Tell the user how/if the MODE of FILE has been changed.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, mode_t mode,
		 enum Change_status changed)
{
  char perms[11];		/* "-rwxrwxrwx" ls-style modes. */
  const char *fmt;

  mode_string (mode, perms);
  perms[10] = '\0';		/* `mode_string' does not null terminate. */
  switch (changed)
    {
    case CH_SUCCEEDED:
      fmt = _("mode of %s changed to %04lo (%s)\n");
      break;
    case CH_FAILED:
      fmt = _("failed to change mode of %s to %04lo (%s)\n");
      break;
    case CH_NO_CHANGE_REQUESTED:
      fmt = _("mode of %s retained as %04lo (%s)\n");
      break;
    default:
      abort ();
    }
  printf (fmt, quote (file),
	  (unsigned long) (mode & CHMOD_MODE_BITS), &perms[1]);
}

/* Change the mode of FILE according to the list of operations CHANGES.
   Return 0 if successful, 1 if errors occurred.  This function is called
   once for every file system object that fts encounters.  */

static int
process_file (FTS *fts, FTSENT *ent, const struct mode_change *changes)
{
  const char *file_full_name = ent->fts_path;
  const char *file = ent->fts_accpath;
  const struct stat *sb = ent->fts_statp;
  mode_t newmode;
  int errors = 0;
  int fail;
  int saved_errno;

  switch (ent->fts_info)
    {
    case FTS_NS:
      error (0, ent->fts_errno, _("cannot access %s"), quote (file_full_name));
      return 1;

    case FTS_ERR:
      error (0, ent->fts_errno, _("%s"), quote (file_full_name));
      return 1;

    case FTS_DNR:
      error (0, ent->fts_errno, _("cannot read directory %s"),
	     quote (file_full_name));
      return 1;

    default:
      break;
    }

  /* If this is the second (post-order) encounter with a directory,
     then return right away.  */
  if (ent->fts_info == FTS_DP)
    return 0;

  if (ROOT_DEV_INO_CHECK (root_dev_ino, sb))
    {
      ROOT_DEV_INO_WARN (file_full_name);
      return 1;
    }

  if (S_ISLNK (sb->st_mode))
    return 0;

  newmode = mode_adjust (sb->st_mode, changes);

  fail = chmod (file, newmode);
  saved_errno = errno;

  if (verbosity == V_high
      || (verbosity == V_changes_only
	  && !fail && mode_changed (file, sb->st_mode)))
    describe_change (file_full_name, newmode,
		     (fail ? CH_FAILED : CH_SUCCEEDED));

  if (fail)
    {
      if (force_silent == 0)
	error (0, saved_errno, _("changing permissions of %s"),
	       quote (file_full_name));
      errors = 1;
    }

  if ( ! recurse)
    fts_set (fts, ent, FTS_SKIP);

  return errors;
}

/* Recursively change the modes of the specified FILES (the last entry
   of which is NULL) according to the list of operations CHANGES.
   BIT_FLAGS controls how fts works.
   If the fts_open call fails, exit nonzero.
   Otherwise, return nonzero upon error.  */

static int
process_files (char **files, int bit_flags, const struct mode_change *changes)
{
  int fail = 0;

  FTS *fts = xfts_open (files, bit_flags, NULL);

  while (1)
    {
      FTSENT *ent;

      ent = fts_read (fts);
      if (ent == NULL)
	{
	  if (errno != 0)
	    {
	      /* FIXME: try to give a better message  */
	      error (0, errno, _("fts_read failed"));
	      fail = 1;
	    }
	  break;
	}

      fail |= process_file (fts, ent, changes);
    }

  /* Ignore failure, since the only way it can do so is in failing to
     return to the original directory, and since we're about to exit,
     that doesn't matter.  */
  fts_close (fts);

  return fail;
}

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... MODE[,MODE]... FILE...\n\
  or:  %s [OPTION]... OCTAL-MODE FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name, program_name);
      fputs (_("\
Change the mode of each FILE to MODE.\n\
\n\
  -c, --changes           like verbose but report only when a change is made\n\
"), stdout);
      fputs (_("\
      --no-preserve-root  do not treat `/' specially (the default)\n\
      --preserve-root     fail to operate recursively on `/'\n\
"), stdout);
      fputs (_("\
  -f, --silent, --quiet   suppress most error messages\n\
  -v, --verbose           output a diagnostic for every file processed\n\
      --reference=RFILE   use RFILE's mode instead of MODE values\n\
  -R, --recursive         change files and directories recursively\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Each MODE is one or more of the letters ugoa, one of the symbols +-= and\n\
one or more of the letters rwxXstugo.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

/* Parse the ASCII mode given on the command line into a linked list
   of `struct mode_change' and apply that to each file argument. */

int
main (int argc, char **argv)
{
  struct mode_change *changes;
  int fail = 0;
  int modeind = 0;		/* Index of the mode argument in `argv'. */
  int thisind;
  bool preserve_root = false;
  int c;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  recurse = force_silent = 0;

  while (1)
    {
      thisind = optind ? optind : 1;

      c = getopt_long (argc, argv, "RcfvrwxXstugoa,+-=", long_options, NULL);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'r':
	case 'w':
	case 'x':
	case 'X':
	case 's':
	case 't':
	case 'u':
	case 'g':
	case 'o':
	case 'a':
	case ',':
	case '+':
	case '-':
	case '=':
	  if (modeind != 0 && modeind != thisind)
	    {
	      static char char_string[2] = {0, 0};
	      char_string[0] = c;
	      error (EXIT_FAILURE, 0,
		     _("invalid character %s in mode string %s"),
		     quote_n (0, char_string), quote_n (1, argv[thisind]));
	    }
	  modeind = thisind;
	  break;
	case NO_PRESERVE_ROOT:
	  preserve_root = false;
	  break;
	case PRESERVE_ROOT:
	  preserve_root = true;
	  break;
	case REFERENCE_FILE_OPTION:
	  reference_file = optarg;
	  break;
	case 'R':
	  recurse = 1;
	  break;
	case 'c':
	  verbosity = V_changes_only;
	  break;
	case 'f':
	  force_silent = 1;
	  break;
	case 'v':
	  verbosity = V_high;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (modeind == 0 && reference_file == NULL)
    modeind = optind++;

  if (optind >= argc)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  changes = (reference_file ? mode_create_from_ref (reference_file)
	     : mode_compile (argv[modeind], MODE_MASK_ALL));

  if (changes == MODE_INVALID)
    error (EXIT_FAILURE, 0,
	   _("invalid mode string: %s"), quote (argv[modeind]));
  else if (changes == MODE_MEMORY_EXHAUSTED)
    xalloc_die ();
  else if (changes == MODE_BAD_REFERENCE)
    error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
	   quote (reference_file));

  if (recurse && preserve_root)
    {
      static struct dev_ino dev_ino_buf;
      root_dev_ino = get_root_dev_ino (&dev_ino_buf);
      if (root_dev_ino == NULL)
	error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
	       quote ("/"));
    }
  else
    {
      root_dev_ino = NULL;
    }

  fail = process_files (argv + optind, FTS_COMFOLLOW, changes);

  exit (fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
