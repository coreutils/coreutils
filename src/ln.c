/* `ln' program to create links between files.
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

/* Written by Mike Parker and David MacKenzie. */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "same.h"
#include "backupfile.h"
#include "dirname.h"
#include "error.h"
#include "quote.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "ln"

#define AUTHORS N_ ("Mike Parker and David MacKenzie")

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  TARGET_DIRECTORY_OPTION = CHAR_MAX + 1
};

int link ();			/* Some systems don't declare this anywhere. */

#ifdef S_ISLNK
int symlink ();
#endif

/* In being careful not even to try to make hard links to directories,
   we have to know whether link(2) follows symlinks.  If it does, then
   we have to *stat* the `source' to see if the resulting link would be
   to a directory.  Otherwise, we have to use *lstat* so that we allow
   users to make hard links to symlinks-that-point-to-directories.  */

#if LINK_FOLLOWS_SYMLINKS
# define STAT_LIKE_LINK(File, Stat_buf) \
  stat (File, Stat_buf)
#else
# define STAT_LIKE_LINK(File, Stat_buf) \
  lstat (File, Stat_buf)
#endif

/* Construct a string NEW_DEST by concatenating DEST, a slash, and
   basename(SOURCE) in alloca'd memory.  Don't modify DEST or SOURCE.  */

#define PATH_BASENAME_CONCAT(new_dest, dest, source)			\
    do									\
      {									\
	const char *source_base;					\
	char *tmp_source;						\
									\
	tmp_source = (char *) alloca (strlen ((source)) + 1);		\
	strcpy (tmp_source, (source));					\
	strip_trailing_slashes (tmp_source);				\
	source_base = base_name (tmp_source);				\
									\
	(new_dest) = (char *) alloca (strlen ((dest)) + 1		\
				      + strlen (source_base) + 1);	\
	stpcpy (stpcpy (stpcpy ((new_dest), (dest)), "/"), source_base);\
      }									\
    while (0)

int isdir ();
int yesno ();

/* The name by which the program was run, for error messages.  */
char *program_name;

/* FIXME: document */
enum backup_type backup_type;

/* A pointer to the function used to make links.  This will point to either
   `link' or `symlink'. */
static int (*linkfunc) ();

/* If nonzero, make symbolic links; otherwise, make hard links.  */
static int symbolic_link;

/* If nonzero, ask the user before removing existing files.  */
static int interactive;

/* If nonzero, remove existing files unconditionally.  */
static int remove_existing_files;

/* If nonzero, list each file as it is moved. */
static int verbose;

/* If nonzero, allow the superuser to make hard links to directories. */
static int hard_dir_link;

/* If nonzero, and the specified destination is a symbolic link to a
   directory, treat it just as if it were a directory.  Otherwise, the
   command `ln --force --no-dereference file symlink-to-dir' deletes
   symlink-to-dir before creating the new link.  */
static int dereference_dest_dir_symlinks = 1;

static struct option const long_options[] =
{
  {"backup", optional_argument, NULL, 'b'},
  {"directory", no_argument, NULL, 'F'},
  {"no-dereference", no_argument, NULL, 'n'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"suffix", required_argument, NULL, 'S'},
  {"target-directory", required_argument, NULL, TARGET_DIRECTORY_OPTION},
  {"symbolic", no_argument, NULL, 's'},
  {"verbose", no_argument, NULL, 'v'},
  {"version-control", required_argument, NULL, 'V'}, /* Deprecated. FIXME. */
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* Make a link DEST to the (usually) existing file SOURCE.
   Symbolic links to nonexistent files are allowed.
   If DEST is a directory, put the link to SOURCE in that directory.
   Return 1 if there is an error, otherwise 0.  */

static int
do_link (const char *source, const char *dest)
{
  struct stat source_stats;
  struct stat dest_stats;
  char *dest_backup = NULL;
  int lstat_status;
  int backup_succeeded = 0;

  /* Use stat here instead of lstat.
     On SVR4, link does not follow symlinks, so this check disallows
     making hard links to symlinks that point to directories.  Big deal.
     On other systems, link follows symlinks, so this check is right.  */
  if (!symbolic_link)
    {
      if (STAT_LIKE_LINK (source, &source_stats) != 0)
	{
	  error (0, errno, _("accessing %s"), quote (source));
	  return 1;
	}

      if (S_ISLNK (source_stats.st_mode))
	{
	  error (0, 0, _("%s: warning: making a hard link to a symbolic link\
 is not portable"),
		 quote (source));
	}

      if (!hard_dir_link && S_ISDIR (source_stats.st_mode))
	{
	  error (0, 0, _("%s: hard link not allowed for directory"),
		 quote (source));
	  return 1;
	}
    }

  lstat_status = lstat (dest, &dest_stats);
  if (lstat_status != 0 && errno != ENOENT)
    {
      error (0, errno, _("accessing %s"), quote (dest));
      return 1;
    }

  /* If the destination is a directory or (it is a symlink to a directory
     and the user has not specified --no-dereference), then form the
     actual destination name by appending base_name (source) to the
     specified destination directory.  */
  if ((lstat_status == 0
       && S_ISDIR (dest_stats.st_mode))
#ifdef S_ISLNK
      || (dereference_dest_dir_symlinks
	  && (lstat_status == 0
	      && S_ISLNK (dest_stats.st_mode)
	      && isdir (dest)))
#endif
     )
    {
      /* Target is a directory; build the full filename. */
      char *new_dest;
      PATH_BASENAME_CONCAT (new_dest, dest, source);
      dest = new_dest;

      /* Get stats for new DEST.  */
      lstat_status = lstat (dest, &dest_stats);
      if (lstat_status != 0 && errno != ENOENT)
	{
	  error (0, errno, _("accessing %s"), quote (dest));
	  return 1;
	}
    }

  /* If --force (-f) has been specified without --backup, then before
     making a link ln must remove the destination file if it exists.
     (with --backup, it just renames any existing destination file)
     But if the source and destination are the same, don't remove
     anything and fail right here.  */
  if (remove_existing_files
      && lstat_status == 0
      /* Allow `ln -sf --backup k k' to succeed in creating the
	 self-referential symlink, but don't allow the hard-linking
	 equivalent: `ln -f k k' (with or without --backup) to get
	 beyond this point, because the error message you'd get is
	 misleading.  */
      && (backup_type == none || !symbolic_link)
      && (!symbolic_link || stat (source, &source_stats) == 0)
      && source_stats.st_dev == dest_stats.st_dev
      && source_stats.st_ino == dest_stats.st_ino
      /* The following detects whether removing DEST will also remove
 	 SOURCE.  If the file has only one link then both are surely
 	 the same link.  Otherwise check whether they point to the same
	 name in the same directory.  */
      && (source_stats.st_nlink == 1 || same_name (source, dest)))
    {
      error (0, 0, _("%s and %s are the same file"),
	     quote_n (0, source), quote_n (1, dest));
      return 1;
    }

  if (lstat_status == 0 || lstat (dest, &dest_stats) == 0)
    {
      if (S_ISDIR (dest_stats.st_mode))
	{
	  error (0, 0, _("%s: cannot overwrite directory"), quote (dest));
	  return 1;
	}
      if (interactive)
	{
	  fprintf (stderr, _("%s: replace %s? "), program_name, quote (dest));
	  if (!yesno ())
	    return 0;
	}
      else if (!remove_existing_files && backup_type == none)
	{
	  error (0, 0, _("%s: File exists"), quote (dest));
	  return 1;
	}

      if (backup_type != none)
	{
	  char *tmp_backup = find_backup_file_name (dest, backup_type);
	  if (tmp_backup == NULL)
	    xalloc_die ();
	  dest_backup = (char *) alloca (strlen (tmp_backup) + 1);
	  strcpy (dest_backup, tmp_backup);
	  free (tmp_backup);
	  if (rename (dest, dest_backup))
	    {
	      if (errno != ENOENT)
		{
		  error (0, errno, _("cannot backup %s"), quote (dest));
		  return 1;
		}
	      else
		dest_backup = NULL;
	    }
	  else
	    {
	      backup_succeeded = 1;
	    }
	}

      /* Try to unlink DEST even if we may have renamed it.  In some unusual
	 cases (when DEST and DEST_BACKUP are hard-links that refer to the
	 same file), rename succeeds and DEST remains.  If we didn't remove
	 DEST in that case, the subsequent LINKFUNC call would fail.  */
      if (unlink (dest) && errno != ENOENT)
	{
	  error (0, errno, _("cannot remove %s"), quote (dest));
	  return 1;
	}
    }
  else if (errno != ENOENT)
    {
      error (0, errno, _("accessing %s"), quote (dest));
      return 1;
    }

  if (verbose)
    {
      printf ((symbolic_link
	       ? _("create symbolic link %s to %s")
	       : _("create hard link %s to %s")),
	      quote_n (0, dest), quote_n (1, source));
      if (backup_succeeded)
	printf (_(" (backup: %s)"), quote (dest_backup));
      putchar ('\n');
    }

  if ((*linkfunc) (source, dest) == 0)
    {
      return 0;
    }

  error (0, errno,
	 (symbolic_link
	  ? _("creating symbolic link %s to %s")
	  : _("creating hard link %s to %s")),
	 quote_n (0, dest), quote_n (1, source));

  if (dest_backup)
    {
      if (rename (dest_backup, dest))
	error (0, errno, _("cannot un-backup %s"), quote (dest));
    }
  return 1;
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
Usage: %s [OPTION]... TARGET [LINK_NAME]\n\
  or:  %s [OPTION]... TARGET... DIRECTORY\n\
  or:  %s [OPTION]... --target-directory=DIRECTORY TARGET...\n\
"),
	      program_name, program_name, program_name);
      fputs (_("\
Create a link to the specified TARGET with optional LINK_NAME.\n\
If LINK_NAME is omitted, a link with the same basename as the TARGET is\n\
created in the current directory.  When using the second form with more\n\
than one TARGET, the last argument must be a directory;  create links\n\
in DIRECTORY to each TARGET.  Create hard links by default, symbolic\n\
links with --symbolic.  When creating hard links, each TARGET must exist.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
      --backup[=CONTROL]      make a backup of each existing destination file\n\
  -b                          like --backup but does not accept an argument\n\
  -d, -F, --directory         hard link directories (super-user only)\n\
  -f, --force                 remove existing destination files\n\
"), stdout);
      fputs (_("\
  -n, --no-dereference        treat destination that is a symlink to a\n\
                                directory as if it were a normal file\n\
  -i, --interactive           prompt whether to remove destinations\n\
  -s, --symbolic              make symbolic links instead of hard links\n\
"), stdout);
      fputs (_("\
  -S, --suffix=SUFFIX         override the usual backup suffix\n\
      --target-directory=DIRECTORY  specify the DIRECTORY in which to create\n\
                                the links\n\
  -v, --verbose               print name of each file before linking\n\
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
  char *backup_suffix_string;
  char *version_control_string = NULL;
  char *target_directory = NULL;
  int target_directory_specified;
  unsigned int n_files;
  char **file;
  int dest_is_dir;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  symbolic_link = remove_existing_files = interactive = verbose
    = hard_dir_link = 0;
  errors = 0;

  while ((c = getopt_long (argc, argv, "bdfinsvFS:V:", long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:			/* Long-named option. */
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
	case 'd':
	case 'F':
	  hard_dir_link = 1;
	  break;
	case 'f':
	  remove_existing_files = 1;
	  interactive = 0;
	  break;
	case 'i':
	  remove_existing_files = 0;
	  interactive = 1;
	  break;
	case 'n':
	  dereference_dest_dir_symlinks = 0;
	  break;
	case 's':
#ifdef S_ISLNK
	  symbolic_link = 1;
#else
	  error (EXIT_FAILURE, 0,
		 _("symbolic links are not supported on this system"));
#endif
	  break;
	case TARGET_DIRECTORY_OPTION:
	  target_directory = optarg;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case 'S':
	  make_backups = 1;
	  backup_suffix_string = optarg;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	  break;
	}
    }

  n_files = argc - optind;
  file = argv + optind;

  if (n_files == 0)
    {
      error (0, 0, _("missing file argument"));
      usage (EXIT_FAILURE);
    }

  target_directory_specified = (target_directory != NULL);
  if (!target_directory)
    target_directory = file[n_files - 1];

  /* If target directory is not specified, and there's only one
     file argument, then pretend `.' was given as the second argument.  */
  if (!target_directory_specified && n_files == 1)
    {
      static char *dummy[2];
      dummy[0] = file[0];
      dummy[1] = ".";
      file = dummy;
      n_files = 2;
      dest_is_dir = 1;
    }
  else
    {
      dest_is_dir = isdir (target_directory);
    }

  if (symbolic_link)
    linkfunc = symlink;
  else
    linkfunc = link;

  if (target_directory_specified && !dest_is_dir)
    {
      error (0, 0, _("%s: specified target directory is not a directory"),
	     quote (target_directory));
      usage (EXIT_FAILURE);
    }

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  backup_type = (make_backups
		 ? xget_version (_("backup type"), version_control_string)
		 : none);

  if (target_directory_specified || n_files > 2)
    {
      unsigned int i;
      unsigned int last_file_idx = (target_directory_specified
				    ? n_files - 1
				    : n_files - 2);

      if (!target_directory_specified && !dest_is_dir)
	error (EXIT_FAILURE, 0,
	   _("when making multiple links, last argument must be a directory"));
      for (i = 0; i <= last_file_idx; ++i)
	errors += do_link (file[i], target_directory);
    }
  else
    {
      struct stat source_stats;
      const char *source;
      char *dest;
      char *new_dest;

      source = file[0];
      dest = file[1];

      /* When the destination is specified with a trailing slash and the
	 source exists but is not a directory, convert the user's command
	 `ln source dest/' to `ln source dest/basename(source)'.  */

      if (dest[strlen (dest) - 1] == '/'
	  && lstat (source, &source_stats) == 0
	  && !S_ISDIR (source_stats.st_mode))
	{
	  PATH_BASENAME_CONCAT (new_dest, dest, source);
	}
      else
	{
	  new_dest = dest;
	}

      errors = do_link (source, new_dest);
    }

  exit (errors != 0);
}
