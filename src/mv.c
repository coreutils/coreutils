/* mv -- move or rename files
   Copyright (C) 86, 89, 90, 91, 95, 96, 97, 1998 Free Software Foundation, Inc.

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
   -f, --force		Assume a 'y' answer to all questions it would
			normally ask, and not ask the questions.

   -i, --interactive	Require confirmation from the user before
			performing any move that would destroy an
			existing file.

   -u, --update		Do not move a nondirectory that has an
			existing destination with the same or newer
			modification time.

   -v, --verbose		List the name of each file as it is moved, and
			the name it is moved to.

   -b, --backup
   -S, --suffix
   -V, --version-control
			Backup file creation.

   Written by Mike Parker and David MacKenzie */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "path-concat.h"
#include "backupfile.h"
#include "closeout.h"
#include "cp-hash.h"
#include "copy.h"
#include "remove.h"
#include "error.h"

#if HAVE_LCHOWN
# define chown(PATH, OWNER, GROUP) lchown(PATH, OWNER, GROUP)
#endif

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

char *base_name ();
int euidaccess ();
int full_write ();
enum backup_type get_version ();
int isdir ();
int lstat ();
int yesno ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, stdin is a tty. */
static int stdin_tty;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"backup", no_argument, NULL, 'b'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"suffix", required_argument, NULL, 'S'},
  {"update", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {"version-control", required_argument, NULL, 'V'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
rm_option_init (struct rm_options *x)
{
  x->unlink_dirs = 0;

  /* FIXME: maybe this should be 1.  The POSIX spec doesn't specify.  */
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
  x->dereference = 0;
  x->force = 0;
  x->hard_link = 0;
  x->interactive = 0;
  x->myeuid = geteuid ();
  x->one_file_system = 0;
  x->preserve = 1;
  x->require_preserve = 0;  /* FIXME: maybe make this an option */
  x->recursive = 1;
  x->sparse_mode = SPARSE_AUTO;  /* FIXME: maybe make this an option */
  x->symbolic_link = 0;

  /* Find out the current file creation mask, to knock the right bits
     when using chmod.  The creation mask is set to be liberal, so
     that created directories can be written, even if it would not
     have been allowed with the mask this process was started with.  */
  x->umask_kill = 0777777 ^ umask (0);

  x->update = 0;
  x->verbose = 0;
  x->xstat = lstat;
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
  char *dest_backup = NULL;
  struct stat source_stats;
  struct stat dest_stats;
  int nonexistent_dst;
  int fail;

  if (lstat (source, &source_stats) != 0)
    {
      error (0, errno, "%s", source);
      return 1;
    }

  nonexistent_dst = 1;
  if (lstat (dest, &dest_stats) == 0)
    {
      nonexistent_dst = 0;

      if (source_stats.st_dev == dest_stats.st_dev
	  && source_stats.st_ino == dest_stats.st_ino)
	{
	  error (0, 0, _("`%s' and `%s' are the same file"), source, dest);
	  return 1;
	}

      if (S_ISDIR (dest_stats.st_mode))
	{
	  error (0, 0, _("%s: cannot overwrite directory"), dest);
	  return 1;
	}

      if (!S_ISDIR (source_stats.st_mode) && x->update
	  && source_stats.st_mtime <= dest_stats.st_mtime)
	return 0;

      if (!x->force && (x->interactive || stdin_tty)
	  && euidaccess (dest, W_OK))
	{
	  fprintf (stderr, _("%s: replace `%s', overriding mode %04o? "),
		   program_name, dest,
		   (unsigned int) (dest_stats.st_mode & 07777));
	  if (!yesno ())
	    return 0;
	}
      else if (x->interactive)
	{
	  fprintf (stderr, _("%s: replace `%s'? "), program_name, dest);
	  if (!yesno ())
	    return 0;
	}

      if (x->backup_type != none)
	{
	  char *tmp_backup = find_backup_file_name (dest, x->backup_type);
	  if (tmp_backup == NULL)
	    error (1, 0, _("virtual memory exhausted"));
	  dest_backup = (char *) alloca (strlen (tmp_backup) + 1);
	  strcpy (dest_backup, tmp_backup);
	  free (tmp_backup);
	  if (rename (dest, dest_backup))
	    {
	      if (errno != ENOENT)
		{
		  error (0, errno, _("cannot backup `%s'"), dest);
		  return 1;
		}
	      else
		dest_backup = NULL;
	    }
	}
    }
  else if (errno != ENOENT)
    {
      error (0, errno, "%s", dest);
      return 1;
    }

  if (x->verbose)
    printf ("%s -> %s\n", source, dest);

  /* Always try rename first.  */
  fail = rename (source, dest);

  if (fail)
    {
      /* This may mean SOURCE and DEST are on different devices.
	 It may also (conceivably) mean that even though they are
	 on the same device, rename isn't implemented for that device.

	 E.g., (from Joel N. Weber),
	 [...] there might someday be cases where you can't rename but you
	 can copy where the device name is the same, especially on Hurd.
	 Consider an ftpfs with a primitive ftp server that supports
	 uploading, downloading and deleting, but not renaming.

	 Also, note that comparing device numbers is not a reliable check
	 for `can-rename'.  Some systems can be set up so that files from
	 many different physical devices all have the same st_dev field.
	 This is a feature of some NFS mounting configurations.

	 Try copying-then-removing SOURCE instead.

	 This function used to resort to copying only when rename failed
	 and set errno to EXDEV.  */

      static int first = 1;
      int copy_into_self;

      if (first)
	{
	  first = 0;

	  /* Allocate space for remembering copied and created files.  */
	  hash_init (INITIAL_HASH_MODULE, INITIAL_ENTRY_TAB_SIZE);
	}

      fail = copy (source, dest, nonexistent_dst, x, &copy_into_self);
      if (fail)
	{
	  /* Restore original destination file DEST if made a backup.  */
	  if (dest_backup && rename (dest_backup, dest))
	    error (0, errno, _("cannot un-backup `%s'"), dest);
	}
      else if (copy_into_self)
	{
	  /* Do *not* remove SOURCE if it is the same as or a parent of DEST.
	     Otherwise, mv would be removing the original *and* the copy.  */
	}
      else
	{
	  struct rm_options rm_options;
	  struct File_spec fs;
	  enum RM_status status;

	  rm_option_init (&rm_options);
	  rm_options.verbose = x->verbose;

	  remove_init ();

	  fspec_init_file (&fs, source);
	  status = rm (&fs, 1, &rm_options);
	  assert (VALID_STATUS (status));
	  if (status == RM_ERROR)
	    fail = 1;

	  remove_fini ();

	  if (fail)
	    error (0, errno, _("cannot remove `%s'"), source);
	}
    }

  return fail;
}

static int
strip_trailing_slashes_2 (char *path)
{
  char *end_p = path + strlen (path) - 1;
  char *slash = end_p;

  while (slash > path && *slash == '/')
    *slash-- = '\0';

  return slash < end_p;
}

/* Move file SOURCE onto DEST.  Handles the case when DEST is a directory.
   DEST_IS_DIR must be nonzero when DEST is a directory or a symlink to a
   directory and zero otherwise.
   Return 0 if successful, non-zero if an error occurred.  */

static int
movefile (char *source, char *dest, int dest_is_dir, const struct cp_options *x)
{
  int dest_had_trailing_slash = strip_trailing_slashes_2 (dest);
  int fail;

  /* In addition to when DEST is a directory, if DEST has a trailing
     slash and neither SOURCE nor DEST is a directory, presume the target
     is DEST/`basename source`.  This converts `mv x y/' to `mv x y/x'.
     This change means that the command `mv any file/' will now fail
     rather than performing the move.  The case when SOURCE is a
     directory and DEST is not is properly diagnosed by do_move.  */

  if (dest_is_dir || (dest_had_trailing_slash && !is_real_dir (source)))
    {
      /* DEST is a directory; build full target filename. */
      char *src_basename;
      char *new_dest;

      /* Remove trailing slashes before taking base_name.
	 Otherwise, base_name ("a/") returns "".  */
      strip_trailing_slashes_2 (source);

      src_basename = base_name (source);
      new_dest = path_concat (dest, src_basename, NULL);
      if (new_dest == NULL)
	error (1, 0, _("virtual memory exhausted"));
      fail = do_move (source, new_dest, x);
      free (new_dest);
    }
  else
    {
      fail = do_move (source, dest, x);
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
      printf (_("\
Usage: %s [OPTION]... SOURCE DEST\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY\n\
"),
	      program_name, program_name);
      printf (_("\
Rename SOURCE to DEST, or move SOURCE(s) to DIRECTORY.\n\
\n\
  -b, --backup                 make backup before removal\n\
  -f, --force                  remove existing destinations, never prompt\n\
  -i, --interactive            prompt before overwrite\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
  -u, --update                 move only older or brand new non-directories\n\
  -v, --verbose                explain what is being done\n\
  -V, --version-control=WORD   override the usual version control\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
\n\
"));
      printf (_("\
The backup suffix is ~, unless set with SIMPLE_BACKUP_SUFFIX.  The\n\
version control may be set with VERSION_CONTROL, values are:\n\
\n\
  t, numbered     make numbered backups\n\
  nil, existing   numbered if numbered backups exist, simple otherwise\n\
  never, simple   always make simple backups\n\
"));
      puts (_("\nReport bugs to <fileutils-bugs@gnu.org>."));
      close_stdout ();
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
  char *version;
  struct cp_options x;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  cp_option_init (&x);

  version = getenv ("SIMPLE_BACKUP_SUFFIX");
  if (version)
    simple_backup_suffix = version;
  version = getenv ("VERSION_CONTROL");

  errors = 0;

  while ((c = getopt_long (argc, argv, "bfiuvS:V:", long_options, NULL)) != -1)
    {
      switch (c)
	{
	case 0:
	  break;
	case 'b':
	  make_backups = 1;
	  break;
	case 'f':
	  x.interactive = 0;
	  x.force = 1;
	  break;
	case 'i':
	  x.interactive = 1;
	  x.force = 0;
	  break;
	case 'u':
	  x.update = 1;
	  break;
	case 'v':
	  x.verbose = 1;
	  break;
	case 'S':
	  simple_backup_suffix = optarg;
	  break;
	case 'V':
	  version = optarg;
	  break;
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("mv (%s) %s\n", GNU_PACKAGE, VERSION);
      close_stdout ();
      exit (0);
    }

  if (show_help)
    usage (0);

  if (argc < optind + 2)
    {
      error (0, 0, "%s", (argc == optind
			  ? _("missing file arguments")
			  : _("missing file argument")));
      usage (1);
    }

  x.backup_type = (make_backups ? get_version (version) : none);

  stdin_tty = isatty (STDIN_FILENO);
  dest_is_dir = isdir (argv[argc - 1]);

  if (argc > optind + 2 && !dest_is_dir)
    error (1, 0,
	   _("when moving multiple files, last argument must be a directory"));

  /* Move each arg but the last onto the last. */
  for (; optind < argc - 1; ++optind)
    errors |= movefile (argv[optind], argv[argc - 1], dest_is_dir, &x);

  if (x.verbose)
    close_stdout ();
  exit (errors);
}
