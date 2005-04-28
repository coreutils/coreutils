/* chmod -- change permission modes of files
   Copyright (C) 89, 90, 91, 1995-2005 Free Software Foundation, Inc.

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
#include "xfts.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "chmod"

#define AUTHORS "David MacKenzie", "Jim Meyering"

enum Change_status
{
  CH_NOT_APPLIED,
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

/* The desired change to the mode.  */
static struct mode_change *change;

/* The initial umask value, if it might be needed.  */
static mode_t umask_value;

/* If true, change the modes of directories recursively. */
static bool recurse;

/* If true, force silence (no error messages). */
static bool force_silent;

/* Level of verbosity.  */
static enum Verbosity verbosity = V_off;

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
  {"changes", no_argument, NULL, 'c'},
  {"recursive", no_argument, NULL, 'R'},
  {"no-preserve-root", no_argument, NULL, NO_PRESERVE_ROOT},
  {"preserve-root", no_argument, NULL, PRESERVE_ROOT},
  {"quiet", no_argument, NULL, 'f'},
  {"reference", required_argument, NULL, REFERENCE_FILE_OPTION},
  {"silent", no_argument, NULL, 'f'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Return true if the chmodable permission bits of FILE changed.
   The old mode was OLD_MODE, but it was changed to NEW_MODE.  */

static bool
mode_changed (char const *file, mode_t old_mode, mode_t new_mode)
{
  if (new_mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* The new mode contains unusual bits that the call to chmod may
	 have silently cleared.  Check whether they actually changed.  */

      struct stat new_stats;

      if (stat (file, &new_stats) != 0)
	{
	  if (!force_silent)
	    error (0, errno, _("getting new attributes of %s"), quote (file));
	  return false;
	}

      new_mode = new_stats.st_mode;
    }

  return ((old_mode ^ new_mode) & CHMOD_MODE_BITS) != 0;
}

/* Tell the user how/if the MODE of FILE has been changed.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, mode_t mode,
		 enum Change_status changed)
{
  char perms[11];		/* "-rwxrwxrwx" ls-style modes. */
  const char *fmt;

  if (changed == CH_NOT_APPLIED)
    {
      printf (_("neither symbolic link %s nor referent has been changed\n"),
	      quote (file));
      return;
    }

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
	  (unsigned long int) (mode & CHMOD_MODE_BITS), &perms[1]);
}

/* Change the mode of FILE.
   Return true if successful.  This function is called
   once for every file system object that fts encounters.  */

static bool
process_file (FTS *fts, FTSENT *ent)
{
  char const *file_full_name = ent->fts_path;
  char const *file = ent->fts_accpath;
  const struct stat *file_stats = ent->fts_statp;
  mode_t new_mode IF_LINT (= 0);
  bool ok = true;
  bool do_chmod;
  bool symlink_changed = true;

  switch (ent->fts_info)
    {
    case FTS_DP:
      return true;

    case FTS_NS:
      error (0, ent->fts_errno, _("cannot access %s"), quote (file_full_name));
      ok = false;
      break;

    case FTS_ERR:
      error (0, ent->fts_errno, _("%s"), quote (file_full_name));
      ok = false;
      break;

    case FTS_DNR:
      error (0, ent->fts_errno, _("cannot read directory %s"),
	     quote (file_full_name));
      ok = false;
      break;

    default:
      break;
    }

  do_chmod = ok;

  if (do_chmod && ROOT_DEV_INO_CHECK (root_dev_ino, file_stats))
    {
      ROOT_DEV_INO_WARN (file_full_name);
      ok = false;
      do_chmod = false;
    }

  if (do_chmod)
    {
      new_mode = mode_adjust (file_stats->st_mode, change, umask_value);

      if (S_ISLNK (file_stats->st_mode))
	symlink_changed = false;
      else
	{
	  ok = (chmod (file, new_mode) == 0);

	  if (! (ok | force_silent))
	    error (0, errno, _("changing permissions of %s"),
		   quote (file_full_name));
	}
    }

  if (verbosity != V_off)
    {
      bool changed =
	(ok && symlink_changed
	 && mode_changed (file, file_stats->st_mode, new_mode));

      if (changed || verbosity == V_high)
	{
	  enum Change_status ch_status =
	    (!ok ? CH_FAILED
	     : !symlink_changed ? CH_NOT_APPLIED
	     : !changed ? CH_NO_CHANGE_REQUESTED
	     : CH_SUCCEEDED);
	  describe_change (file_full_name, new_mode, ch_status);
	}
    }

  if ( ! recurse)
    fts_set (fts, ent, FTS_SKIP);

  return ok;
}

/* Recursively change the modes of the specified FILES (the last entry
   of which is NULL).  BIT_FLAGS controls how fts works.
   Return true if successful.  */

static bool
process_files (char **files, int bit_flags)
{
  bool ok = true;

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
	      ok = false;
	    }
	  break;
	}

      ok &= process_file (fts, ent);
    }

  /* Ignore failure, since the only way it can do so is in failing to
     return to the original directory, and since we're about to exit,
     that doesn't matter.  */
  fts_close (fts);

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
Each MODE is of the form `[ugoa]*([-+=]([rwxXst]*|[ugo]))+'.\n\
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
  char *mode = NULL;
  size_t mode_len = 0;
  size_t mode_alloc = 0;
  bool ok;
  bool preserve_root = false;
  char const *reference_file = NULL;
  int c;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  recurse = force_silent = false;

  while ((c = getopt_long (argc, argv,
			   "Rcfvr::w::x::X::s::t::u::g::o::a::,::+::-::=::",
			   long_options, NULL))
	 != -1)
    {
      switch (c)
	{
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
	  {
	    /* Allocate a mode string (e.g., "-rwx") by concatenating
	       the argument containing this option.  If a previous mode
	       string was given, concatenate the previous string, a
	       comma, and the new string (e.g., "-s,-rwx").  */

	    char const *arg = argv[optind - 1];
	    size_t arg_len = strlen (arg);
	    size_t mode_comma_len = mode_len + !!mode_len;
	    size_t new_mode_len = mode_comma_len + arg_len;
	    if (mode_alloc <= new_mode_len)
	      {
		mode_alloc = new_mode_len + 1;
		mode = x2realloc (mode, &mode_alloc);
	      }
	    mode[mode_len] = ',';
	    strcpy (mode + mode_comma_len, arg);
	    mode_len = new_mode_len;
	  }
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
	  recurse = true;
	  break;
	case 'c':
	  verbosity = V_changes_only;
	  break;
	case 'f':
	  force_silent = true;
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

  if (reference_file)
    {
      if (mode)
	{
	  error (0, 0, _("cannot combine mode and --reference options"));
	  usage (EXIT_FAILURE);
	}
    }
  else
    {
      if (!mode)
	mode = argv[optind++];
    }

  if (optind >= argc)
    {
      if (!mode || mode != argv[optind - 1])
	error (0, 0, _("missing operand"));
      else
	error (0, 0, _("missing operand after %s"), quote (argv[argc - 1]));
      usage (EXIT_FAILURE);
    }

  if (reference_file)
    {
      change = mode_create_from_ref (reference_file);
      if (!change)
	error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
	       quote (reference_file));
    }
  else
    {
      change = mode_compile (mode);
      if (!change)
	{
	  error (0, 0, _("invalid mode: %s"), quote (mode));
	  usage (EXIT_FAILURE);
	}
      umask_value = umask (0);
    }

  if (recurse & preserve_root)
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

  ok = process_files (argv + optind, FTS_COMFOLLOW);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
