/* mv -- move or rename files
   Copyright (C) 86, 89, 90, 91, 1995-2002 Free Software Foundation, Inc.

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

/* Written by Mike Parker, David MacKenzie, and Jim Meyering */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "argmatch.h"
#include "backupfile.h"
#include "copy.h"
#include "cp-hash.h"
#include "dirname.h"
#include "error.h"
#include "path-concat.h"
#include "quote.h"
#include "remove.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "mv"

#define AUTHORS N_ ("Mike Parker, David MacKenzie, and Jim Meyering")

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TARGET_DIRECTORY_OPTION = CHAR_MAX + 1,
  STRIP_TRAILING_SLASHES_OPTION,
  REPLY_OPTION
};

int isdir ();
int lstat ();

/* The name this program was run with. */
char *program_name;

/* Remove any trailing slashes from each SOURCE argument.  */
static int remove_trailing_slashes;

/* Valid arguments to the `--reply' option. */
static char const* const reply_args[] =
{
  "yes", "no", "query", 0
};

/* The values that correspond to the above strings. */
static int const reply_vals[] =
{
  I_ALWAYS_YES, I_ALWAYS_NO, I_ASK_USER
};

static struct option const long_options[] =
{
  {"backup", optional_argument, NULL, 'b'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"reply", required_argument, NULL, REPLY_OPTION},
  {"strip-trailing-slashes", no_argument, NULL, STRIP_TRAILING_SLASHES_OPTION},
  {"suffix", required_argument, NULL, 'S'},
  {"target-directory", required_argument, NULL, TARGET_DIRECTORY_OPTION},
  {"update", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {"version-control", required_argument, NULL, 'V'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static void
rm_option_init (struct rm_options *x)
{
  x->unlink_dirs = 0;

  x->ignore_missing_files = 0;

  x->recursive = 1;

  /* Should we prompt for removal, too?  No.  Prompting for the `move'
     part is enough.  It implies removal.  */
  x->interactive = 0;
  x->stdin_tty = 0;

  x->verbose = 0;
}

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = 0;  /* FIXME: maybe make this an option */
  x->dereference = DEREF_NEVER;
  x->unlink_dest_before_opening = 0;
  x->unlink_dest_after_failed_open = 0;
  x->hard_link = 0;
  x->interactive = I_UNSPECIFIED;
  x->move_mode = 1;
  x->myeuid = geteuid ();
  x->one_file_system = 0;
  x->preserve_ownership = 1;
  x->preserve_links = 1;
  x->preserve_mode = 1;
  x->preserve_timestamps = 1;
  x->require_preserve = 0;  /* FIXME: maybe make this an option */
  x->recursive = 1;
  x->sparse_mode = SPARSE_AUTO;  /* FIXME: maybe make this an option */
  x->symbolic_link = 0;
  x->set_mode = 0;
  x->mode = 0;
  x->stdin_tty = isatty (STDIN_FILENO);

  /* Find out the current file creation mask, to knock the right bits
     when using chmod.  The creation mask is set to be liberal, so
     that created directories can be written, even if it would not
     have been allowed with the mask this process was started with.  */
  x->umask_kill = ~ umask (0);

  x->update = 0;
  x->verbose = 0;
  x->xstat = lstat;
  x->dest_info = NULL;
  x->src_info = NULL;
}

/* If PATH is an existing directory, return nonzero, else 0.  */

static int
is_real_dir (const char *path)
{
  struct stat stats;

  return lstat (path, &stats) == 0 && S_ISDIR (stats.st_mode);
}

/* Move SOURCE onto DEST.  Handles cross-filesystem moves.
   If SOURCE is a directory, DEST must not exist.
   Return 0 if successful, non-zero if an error occurred.  */

static int
do_move (const char *source, const char *dest, const struct cp_options *x)
{
  static int first = 1;
  int copy_into_self;
  int rename_succeeded;
  int fail;

  if (first)
    {
      first = 0;

      /* Allocate space for remembering copied and created files.  */
      hash_init ();
    }

  fail = copy (source, dest, 0, x, &copy_into_self, &rename_succeeded);

  if (!fail)
    {
      char const *dir_to_remove;
      if (copy_into_self)
	{
	  /* In general, when copy returns with copy_into_self set, SOURCE is
	     the same as, or a parent of DEST.  In this case we know it's a
	     parent.  It doesn't make sense to move a directory into itself, and
	     besides in some situations doing so would give highly nonintuitive
	     results.  Run this `mkdir b; touch a c; mv * b' in an empty
	     directory.  Here's the result of running echo `find b -print`:
	     b b/a b/b b/b/a b/c.  Notice that only file `a' was copied
	     into b/b.  Handle this by giving a diagnostic, removing the
	     copied-into-self directory, DEST (`b/b' in the example),
	     and failing.  */

	  dir_to_remove = NULL;
	  fail = 1;
	}
      else if (rename_succeeded)
	{
	  /* No need to remove anything.  SOURCE was successfully
	     renamed to DEST.  Or the user declined to rename a file.  */
	  dir_to_remove = NULL;
	}
      else
	{
	  /* This may mean SOURCE and DEST referred to different devices.
	     It may also conceivably mean that even though they referred
	     to the same device, rename wasn't implemented for that device.

	     E.g., (from Joel N. Weber),
	     [...] there might someday be cases where you can't rename
	     but you can copy where the device name is the same, especially
	     on Hurd.  Consider an ftpfs with a primitive ftp server that
	     supports uploading, downloading and deleting, but not renaming.

	     Also, note that comparing device numbers is not a reliable
	     check for `can-rename'.  Some systems can be set up so that
	     files from many different physical devices all have the same
	     st_dev field.  This is a feature of some NFS mounting
	     configurations.

	     We reach this point if SOURCE has been successfully copied
	     to DEST.  Now we have to remove SOURCE.

	     This function used to resort to copying only when rename
	     failed and set errno to EXDEV.  */

	  dir_to_remove = source;
	}

      if (dir_to_remove != NULL)
	{
	  struct rm_options rm_options;
	  enum RM_status status;

	  rm_option_init (&rm_options);
	  rm_options.verbose = x->verbose;

	  status = rm (1, &dir_to_remove, &rm_options);
	  assert (VALID_STATUS (status));
	  if (status == RM_ERROR)
	    fail = 1;
	}
    }

  return fail;
}

/* Move file SOURCE onto DEST.  Handles the case when DEST is a directory.
   DEST_IS_DIR must be nonzero when DEST is a directory or a symlink to a
   directory and zero otherwise.
   Return 0 if successful, non-zero if an error occurred.  */

static int
movefile (char *source, char *dest, int dest_is_dir,
	  const struct cp_options *x)
{
  int dest_had_trailing_slash = strip_trailing_slashes (dest);
  int fail;

  /* This code was introduced to handle the ambiguity in the semantics
     of mv that is induced by the varying semantics of the rename function.
     Some systems (e.g., Linux) have a rename function that honors a
     trailing slash, while others (like Solaris 5,6,7) have a rename
     function that ignores a trailing slash.  I believe the Linux
     rename semantics are POSIX and susv2 compliant.  */

  if (remove_trailing_slashes)
    strip_trailing_slashes (source);

  /* In addition to when DEST is a directory, if DEST has a trailing
     slash and neither SOURCE nor DEST is a directory, presume the target
     is DEST/`basename source`.  This converts `mv x y/' to `mv x y/x'.
     This change means that the command `mv any file/' will now fail
     rather than performing the move.  The case when SOURCE is a
     directory and DEST is not is properly diagnosed by do_move.  */

  if (dest_is_dir || (dest_had_trailing_slash && !is_real_dir (source)))
    {
      /* DEST is a directory; build full target filename. */
      char const *src_basename = base_name (source);
      char *new_dest = path_concat (dest, src_basename, NULL);
      if (new_dest == NULL)
	xalloc_die ();
      strip_trailing_slashes (new_dest);
      fail = do_move (source, new_dest, x);
      free (new_dest);
    }
  else
    {
      fail = do_move (source, dest, x);
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
      printf (_("\
Usage: %s [OPTION]... SOURCE DEST\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY\n\
  or:  %s [OPTION]... --target-directory=DIRECTORY SOURCE...\n\
"),
	      program_name, program_name, program_name);
      fputs (_("\
Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
      --backup[=CONTROL]       make a backup of each existing destination file\n\
  -b                           like --backup but does not accept an argument\n\
  -f, --force                  do not prompt before overwriting\n\
                                 equivalent to --reply=yes\n\
  -i, --interactive            prompt before overwrite\n\
                                 equivalent to --reply=query\n\
"), stdout);
      fputs (_("\
      --reply={yes,no,query}   specify how to handle the prompt about an\n\
                                 existing destination file\n\
      --strip-trailing-slashes remove any trailing slashes from each SOURCE\n\
                                 argument\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
"), stdout);
      fputs (_("\
      --target-directory=DIRECTORY  move all SOURCE arguments into DIRECTORY\n\
  -u, --update                 move only when the SOURCE file is newer\n\
                                 than the destination file or when the\n\
                                 destination file is missing\n\
  -v, --verbose                explain what is being done\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control method may be selected via the --backup option or through\n\
the VERSION_CONTROL environment variable.  Here are the values:\n\
\n\
"), stdout);
      fputs (_("\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int c;
  int errors;
  int make_backups = 0;
  int dest_is_dir;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  struct cp_options x;
  char *target_directory = NULL;
  int target_directory_specified;
  unsigned int n_files;
  char **file;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  cp_option_init (&x);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  errors = 0;

  while ((c = getopt_long (argc, argv, "bfiuvS:V:", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'V':  /* FIXME: this is deprecated.  Remove it in 2001.  */
	  error (0, 0,
		 _("warning: --version-control (-V) is obsolete;  support for\
 it\nwill be removed in some future release.  Use --backup=%s instead."
		   ), optarg);
	  /* Fall through.  */

	case 'b':
	  make_backups = 1;
	  if (optarg)
	    version_control_string = optarg;
	  break;
	case 'f':
	  x.interactive = I_ALWAYS_YES;
	  break;
	case 'i':
	  x.interactive = I_ASK_USER;
	  break;
	case REPLY_OPTION:
	  x.interactive = XARGMATCH ("--reply", optarg,
				     reply_args, reply_vals);
	  break;
	case STRIP_TRAILING_SLASHES_OPTION:
	  remove_trailing_slashes = 1;
	  break;
	case TARGET_DIRECTORY_OPTION:
	  target_directory = optarg;
	  break;
	case 'u':
	  x.update = 1;
	  break;
	case 'v':
	  x.verbose = 1;
	  break;
	case 'S':
	  make_backups = 1;
	  backup_suffix_string = optarg;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  n_files = argc - optind;
  file = argv + optind;

  target_directory_specified = (target_directory != NULL);
  if (target_directory == NULL && n_files != 0)
    target_directory = file[n_files - 1];

  dest_is_dir = (n_files > 0 && isdir (target_directory));

  if (n_files == 0 || (n_files == 1 && !target_directory_specified))
    {
      error (0, 0, _("missing file argument"));
      usage (EXIT_FAILURE);
    }

  if (target_directory_specified)
    {
      if (!dest_is_dir)
	{
	  error (0, 0, _("specified target, %s is not a directory"),
		 quote (target_directory));
	  usage (EXIT_FAILURE);
	}
    }
  else if (n_files > 2 && !dest_is_dir)
    {
      error (0, 0,
	    _("when moving multiple files, last argument must be a directory"));
      usage (EXIT_FAILURE);
    }

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  x.backup_type = (make_backups
		   ? xget_version (_("backup type"),
				   version_control_string)
		   : none);

  /* Move each arg but the last into the target_directory.  */
  {
    unsigned int last_file_idx = (target_directory_specified
				  ? n_files - 1
				  : n_files - 2);
    unsigned int i;

    /* Initialize the hash table only if we'll need it.
       The problem it is used to detect can arise only if there are
       two or more files to move.  */
    if (last_file_idx)
      dest_info_init (&x);

    for (i = 0; i <= last_file_idx; ++i)
      errors |= movefile (file[i], target_directory, dest_is_dir, &x);
  }

  exit (errors);
}
