/* `ln' program to create links between files.
   Copyright (C) 86, 89, 90, 91, 1995-2005 Free Software Foundation, Inc.

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

/* Written by Mike Parker and David MacKenzie. */

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
#include "yesno.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "ln"

#define AUTHORS "Mike Parker", "David MacKenzie"

#ifndef ENABLE_HARD_LINK_TO_SYMLINK_WARNING
# define ENABLE_HARD_LINK_TO_SYMLINK_WARNING 0
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
   basename (SOURCE) in alloca'd memory.  Don't modify DEST or SOURCE.
   Omit unnecessary slashes in the boundary between DEST and SOURCE in
   the result; they can cause harm if "/" and "//" denote different
   directories.  */

#define FILE_BASENAME_CONCAT(new_dest, dest, source)			\
    do									\
      {									\
	const char *source_base;					\
	char *tmp_source;						\
	size_t buf_len = strlen (source) + 1;				\
	size_t dest_len = strlen (dest);				\
									\
	tmp_source = alloca (buf_len);					\
	memcpy (tmp_source, (source), buf_len);				\
	strip_trailing_slashes (tmp_source);				\
	source_base = base_name (tmp_source);				\
	source_base += (source_base[0] == '/');				\
	dest_len -= (dest_len != 0 && (dest)[dest_len - 1] == '/');	\
	(new_dest) = alloca (dest_len + 1 + strlen (source_base) + 1);	\
	memcpy (new_dest, dest, dest_len);				\
	(new_dest)[dest_len] = '/';					\
	strcpy ((new_dest) + dest_len + 1, source_base);		\
      }									\
    while (0)

/* The name by which the program was run, for error messages.  */
char *program_name;

/* FIXME: document */
static enum backup_type backup_type;

/* A pointer to the function used to make links.  This will point to either
   `link' or `symlink'. */
static int (*linkfunc) ();

/* If true, make symbolic links; otherwise, make hard links.  */
static bool symbolic_link;

/* If true, ask the user before removing existing files.  */
static bool interactive;

/* If true, remove existing files unconditionally.  */
static bool remove_existing_files;

/* If true, list each file as it is moved. */
static bool verbose;

/* If true, allow the superuser to *attempt* to make hard links
   to directories.  However, it appears that this option is not useful
   in practice, since even the superuser is prohibited from hard-linking
   directories on most (all?) existing systems.  */
static bool hard_dir_link;

/* If nonzero, and the specified destination is a symbolic link to a
   directory, treat it just as if it were a directory.  Otherwise, the
   command `ln --force --no-dereference file symlink-to-dir' deletes
   symlink-to-dir before creating the new link.  */
static bool dereference_dest_dir_symlinks = true;

static struct option const long_options[] =
{
  {"backup", optional_argument, NULL, 'b'},
  {"directory", no_argument, NULL, 'F'},
  {"no-dereference", no_argument, NULL, 'n'},
  {"no-target-directory", no_argument, NULL, 'T'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"suffix", required_argument, NULL, 'S'},
  {"target-directory", required_argument, NULL, 't'},
  {"symbolic", no_argument, NULL, 's'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

/* FILE is the last operand of this command.  Return true if FILE is a
   directory.  But report an error there is a problem accessing FILE,
   or if FILE does not exist but would have to refer to an existing
   directory if it referred to anything at all.  */

static bool
target_directory_operand (char const *file)
{
  char const *b = base_name (file);
  size_t blen = strlen (b);
  bool looks_like_a_dir = (blen == 0 || ISSLASH (b[blen - 1]));
  struct stat st;
  int err = ((dereference_dest_dir_symlinks ? stat : lstat) (file, &st) == 0
	     ? 0 : errno);
  bool is_a_dir = !err && S_ISDIR (st.st_mode);
  if (err && err != ENOENT)
    error (EXIT_FAILURE, err, _("accessing %s"), quote (file));
  if (is_a_dir < looks_like_a_dir)
    error (EXIT_FAILURE, err, _("target %s is not a directory"), quote (file));
  return is_a_dir;
}

/* Make a link DEST to the (usually) existing file SOURCE.
   Symbolic links to nonexistent files are allowed.
   If DEST_IS_DIR, put the link to SOURCE in the DEST directory.
   Return true if successful.  */

static bool
do_link (const char *source, const char *dest, bool dest_is_dir)
{
  struct stat source_stats;
  struct stat dest_stats;
  char *dest_backup = NULL;
  bool lstat_ok = false;

  /* Use stat here instead of lstat.
     On SVR4, link does not follow symlinks, so this check disallows
     making hard links to symlinks that point to directories.  Big deal.
     On other systems, link follows symlinks, so this check is right.  */
  if (!symbolic_link)
    {
      if (STAT_LIKE_LINK (source, &source_stats) != 0)
	{
	  error (0, errno, _("accessing %s"), quote (source));
	  return false;
	}

      if (ENABLE_HARD_LINK_TO_SYMLINK_WARNING
	  && S_ISLNK (source_stats.st_mode))
	{
	  error (0, 0, _("%s: warning: making a hard link to a symbolic link\
 is not portable"),
		 quote (source));
	}

      if (!hard_dir_link && S_ISDIR (source_stats.st_mode))
	{
	  error (0, 0, _("%s: hard link not allowed for directory"),
		 quote (source));
	  return false;
	}
    }

  if (dest_is_dir)
    {
      /* Treat DEST as a directory; build the full filename.  */
      char *new_dest;
      FILE_BASENAME_CONCAT (new_dest, dest, source);
      dest = new_dest;
    }

  if (remove_existing_files || interactive || backup_type != no_backups)
    {
      lstat_ok = (lstat (dest, &dest_stats) == 0);
      if (!lstat_ok && errno != ENOENT)
	{
	  error (0, errno, _("accessing %s"), quote (dest));
	  return false;
	}
    }

  /* If --force (-f) has been specified without --backup, then before
     making a link ln must remove the destination file if it exists.
     (with --backup, it just renames any existing destination file)
     But if the source and destination are the same, don't remove
     anything and fail right here.  */
  if (remove_existing_files
      && lstat_ok
      /* Allow `ln -sf --backup k k' to succeed in creating the
	 self-referential symlink, but don't allow the hard-linking
	 equivalent: `ln -f k k' (with or without --backup) to get
	 beyond this point, because the error message you'd get is
	 misleading.  */
      && (backup_type == no_backups || !symbolic_link)
      && (!symbolic_link || stat (source, &source_stats) == 0)
      && SAME_INODE (source_stats, dest_stats)
      /* The following detects whether removing DEST will also remove
	 SOURCE.  If the file has only one link then both are surely
	 the same link.  Otherwise check whether they point to the same
	 name in the same directory.  */
      && (source_stats.st_nlink == 1 || same_name (source, dest)))
    {
      error (0, 0, _("%s and %s are the same file"),
	     quote_n (0, source), quote_n (1, dest));
      return false;
    }

  if (lstat_ok)
    {
      if (S_ISDIR (dest_stats.st_mode))
	{
	  error (0, 0, _("%s: cannot overwrite directory"), quote (dest));
	  return false;
	}
      if (interactive)
	{
	  fprintf (stderr, _("%s: replace %s? "), program_name, quote (dest));
	  if (!yesno ())
	    return true;
	  remove_existing_files = true;
	}

      if (backup_type != no_backups)
	{
	  char *tmp_backup = find_backup_file_name (dest, backup_type);
	  size_t buf_len = strlen (tmp_backup) + 1;
	  dest_backup = alloca (buf_len);
	  memcpy (dest_backup, tmp_backup, buf_len);
	  free (tmp_backup);
	  if (rename (dest, dest_backup))
	    {
	      if (errno != ENOENT)
		{
		  error (0, errno, _("cannot backup %s"), quote (dest));
		  return false;
		}
	      else
		dest_backup = NULL;
	    }
	}
    }

  if (verbose)
    {
      printf ((symbolic_link
	       ? _("create symbolic link %s to %s")
	       : _("create hard link %s to %s")),
	      quote_n (0, dest), quote_n (1, source));
      if (dest_backup)
	printf (_(" (backup: %s)"), quote (dest_backup));
      putchar ('\n');
    }

  if ((*linkfunc) (source, dest) == 0)
    return true;

  /* If the attempt to create a link failed and we are removing or
     backing up destinations, unlink the destination and try again.

     POSIX 1003.1-2004 requires that ln -f A B must unlink B even on
     failure (e.g., when A does not exist).  This is counterintuitive,
     and we submitted a defect report
     <http://www.opengroup.org/sophocles/show_mail.tpl?source=L&listname=austin-review-l&id=1795>
     (2004-06-24).  If the committee does not fix the standard we'll
     have to change the behavior of ln -f, at least if POSIXLY_CORRECT
     is set.  In the meantime ln -f A B will not unlink B unless the
     attempt to link A to B failed because B already existed.

     Try to unlink DEST even if we may have backed it up successfully.
     In some unusual cases (when DEST and DEST_BACKUP are hard-links
     that refer to the same file), rename succeeds and DEST remains.
     If we didn't remove DEST in that case, the subsequent LINKFUNC
     call would fail.  */

  if (errno == EEXIST && (remove_existing_files || dest_backup))
    {
      if (unlink (dest) != 0)
	{
	  error (0, errno, _("cannot remove %s"), quote (dest));
	  return false;
	}

      if (linkfunc (source, dest) == 0)
	return true;
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
  return false;
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
Usage: %s [OPTION]... [-T] TARGET LINK_NAME   (1st form)\n\
  or:  %s [OPTION]... TARGET                  (2nd form)\n\
  or:  %s [OPTION]... TARGET... DIRECTORY     (3rd form)\n\
  or:  %s [OPTION]... -t DIRECTORY TARGET...  (4th form)\n\
"),
	      program_name, program_name, program_name, program_name);
      fputs (_("\
In the 1st form, create a link to TARGET with the name LINK_NAME.\n\
In the 2nd form, create a link to TARGET in the current directory.\n\
In the 3rd and 4th forms, create links to each TARGET in DIRECTORY.\n\
Create hard links by default, symbolic links with --symbolic.\n\
When creating hard links, each TARGET must exist.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
      --backup[=CONTROL]      make a backup of each existing destination file\n\
  -b                          like --backup but does not accept an argument\n\
  -d, -F, --directory         allow the superuser to attempt to hard link\n\
                                directories (note: will probably fail due to\n\
                                system restrictions, even for the superuser)\n\
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
  -t, --target-directory=DIRECTORY  specify the DIRECTORY in which to create\n\
                                the links\n\
  -T, --no-target-directory   treat LINK_NAME as a normal file\n\
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
  bool ok;
  bool make_backups = false;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  char const *target_directory = NULL;
  bool no_target_directory = false;
  int n_files;
  char **file;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  symbolic_link = remove_existing_files = interactive = verbose
    = hard_dir_link = false;

  while ((c = getopt_long (argc, argv, "bdfinst:vFS:T", long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 'b':
	  make_backups = true;
	  if (optarg)
	    version_control_string = optarg;
	  break;
	case 'd':
	case 'F':
	  hard_dir_link = true;
	  break;
	case 'f':
	  remove_existing_files = true;
	  interactive = false;
	  break;
	case 'i':
	  remove_existing_files = false;
	  interactive = true;
	  break;
	case 'n':
	  dereference_dest_dir_symlinks = false;
	  break;
	case 's':
#ifdef S_ISLNK
	  symbolic_link = true;
#else
	  error (EXIT_FAILURE, 0,
		 _("symbolic links are not supported on this system"));
#endif
	  break;
	case 't':
	  if (target_directory)
	    error (EXIT_FAILURE, 0, _("multiple target directories specified"));
	  else
	    {
	      struct stat st;
	      if (stat (optarg, &st) != 0)
		error (EXIT_FAILURE, errno, _("accessing %s"), quote (optarg));
	      if (! S_ISDIR (st.st_mode))
		error (EXIT_FAILURE, 0, _("target %s is not a directory"),
		       quote (optarg));
	    }
	  target_directory = optarg;
	  break;
	case 'T':
	  no_target_directory = true;
	  break;
	case 'v':
	  verbose = true;
	  break;
	case 'S':
	  make_backups = true;
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

  if (n_files <= 0)
    {
      error (0, 0, _("missing file operand"));
      usage (EXIT_FAILURE);
    }

  if (no_target_directory)
    {
      if (target_directory)
	error (EXIT_FAILURE, 0,
	       _("Cannot combine --target-directory "
		 "and --no-target-directory"));
      if (n_files != 2)
	{
	  if (n_files < 2)
	    error (0, 0,
		   _("missing destination file operand after %s"),
		   quote (file[0]));
	  else
	    error (0, 0, _("extra operand %s"), quote (file[2]));
	  usage (EXIT_FAILURE);
	}
    }
  else if (!target_directory)
    {
      if (n_files < 2)
	target_directory = ".";
      else if (2 <= n_files && target_directory_operand (file[n_files - 1]))
	target_directory = file[--n_files];
      else if (2 < n_files)
	error (EXIT_FAILURE, 0, _("target %s is not a directory"),
	       quote (file[n_files - 1]));
    }

  if (symbolic_link)
    linkfunc = symlink;
  else
    linkfunc = link;

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  backup_type = (make_backups
		 ? xget_version (_("backup type"), version_control_string)
		 : no_backups);

  if (target_directory)
    {
      int i;
      ok = true;
      for (i = 0; i < n_files; ++i)
	ok &= do_link (file[i], target_directory, true);
    }
  else
    ok = do_link (file[0], file[1], false);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
