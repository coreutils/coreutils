/* install - copy files and set attributes
   Copyright (C) 89, 90, 91, 1995-2007 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#include "system.h"
#include "backupfile.h"
#include "error.h"
#include "cp-hash.h"
#include "copy.h"
#include "filenamecat.h"
#include "mkancesdirs.h"
#include "mkdir-p.h"
#include "modechange.h"
#include "quote.h"
#include "savewd.h"
#include "stat-time.h"
#include "utimens.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "install"

#define AUTHORS "David MacKenzie"

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if ! HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#if ! HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

/* Number of bytes of a file to copy at a time. */
#define READ_SIZE (32 * 1024)

static bool change_timestamps (struct stat const *from_sb, char const *to);
static bool change_attributes (char const *name);
static bool copy_file (const char *from, const char *to,
		       const struct cp_options *x);
static bool install_file_in_file_parents (char const *from, char *to,
					  struct cp_options *x);
static bool install_file_in_dir (const char *from, const char *to_dir,
				 const struct cp_options *x);
static bool install_file_in_file (const char *from, const char *to,
				  const struct cp_options *x);
static void get_ids (void);
static void strip (char const *name);
static void announce_mkdir (char const *dir, void *options);
static int make_ancestor (char const *dir, char const *component,
			  void *options);
void usage (int status);

/* The name this program was run with, for error messages. */
char *program_name;

/* The user name that will own the files, or NULL to make the owner
   the current user ID. */
static char *owner_name;

/* The user ID corresponding to `owner_name'. */
static uid_t owner_id;

/* The group name that will own the files, or NULL to make the group
   the current group ID. */
static char *group_name;

/* The group ID corresponding to `group_name'. */
static gid_t group_id;

#define DEFAULT_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* The file mode bits to which non-directory files will be set.  The umask has
   no effect. */
static mode_t mode = DEFAULT_MODE;

/* Similar, but for directories.  */
static mode_t dir_mode = DEFAULT_MODE;

/* The file mode bits that the user cares about.  This should be a
   superset of DIR_MODE and a subset of CHMOD_MODE_BITS.  This matters
   for directories, since otherwise directories may keep their S_ISUID
   or S_ISGID bits.  */
static mode_t dir_mode_bits = CHMOD_MODE_BITS;

/* If true, strip executable files after copying them. */
static bool strip_files;

/* If true, install a directory instead of a regular file. */
static bool dir_arg;

static struct option const long_options[] =
{
  {"backup", optional_argument, NULL, 'b'},
  {"directory", no_argument, NULL, 'd'},
  {"group", required_argument, NULL, 'g'},
  {"mode", required_argument, NULL, 'm'},
  {"no-target-directory", no_argument, NULL, 'T'},
  {"owner", required_argument, NULL, 'o'},
  {"preserve-timestamps", no_argument, NULL, 'p'},
  {"strip", no_argument, NULL, 's'},
  {"suffix", required_argument, NULL, 'S'},
  {"target-directory", required_argument, NULL, 't'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = true;
  x->dereference = DEREF_ALWAYS;
  x->unlink_dest_before_opening = true;
  x->unlink_dest_after_failed_open = false;
  x->hard_link = false;
  x->interactive = I_UNSPECIFIED;
  x->move_mode = false;
  x->chown_privileges = chown_privileges ();
  x->one_file_system = false;
  x->preserve_ownership = false;
  x->preserve_links = false;
  x->preserve_mode = false;
  x->preserve_timestamps = false;
  x->require_preserve = false;
  x->recursive = false;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = false;
  x->backup_type = no_backups;

  /* Create destination files initially writable so we can run strip on them.
     Although GNU strip works fine on read-only files, some others
     would fail.  */
  x->set_mode = true;
  x->mode = S_IRUSR | S_IWUSR;
  x->stdin_tty = false;

  x->update = false;
  x->verbose = false;
  x->dest_info = NULL;
  x->src_info = NULL;
}

/* FILE is the last operand of this command.  Return true if FILE is a
   directory.  But report an error there is a problem accessing FILE,
   or if FILE does not exist but would have to refer to an existing
   directory if it referred to anything at all.  */

static bool
target_directory_operand (char const *file)
{
  char const *b = last_component (file);
  size_t blen = strlen (b);
  bool looks_like_a_dir = (blen == 0 || ISSLASH (b[blen - 1]));
  struct stat st;
  int err = (stat (file, &st) == 0 ? 0 : errno);
  bool is_a_dir = !err && S_ISDIR (st.st_mode);
  if (err && err != ENOENT)
    error (EXIT_FAILURE, err, _("accessing %s"), quote (file));
  if (is_a_dir < looks_like_a_dir)
    error (EXIT_FAILURE, err, _("target %s is not a directory"), quote (file));
  return is_a_dir;
}

/* Process a command-line file name, for the -d option.  */
static int
process_dir (char *dir, struct savewd *wd, void *options)
{
  return (make_dir_parents (dir, wd,
			    make_ancestor, options,
			    dir_mode, announce_mkdir,
			    dir_mode_bits, owner_id, group_id, false)
	  ? EXIT_SUCCESS
	  : EXIT_FAILURE);
}

int
main (int argc, char **argv)
{
  int optc;
  int exit_status = EXIT_SUCCESS;
  const char *specified_mode = NULL;
  bool make_backups = false;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  bool mkdir_and_install = false;
  struct cp_options x;
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

  cp_option_init (&x);

  owner_name = NULL;
  group_name = NULL;
  strip_files = false;
  dir_arg = false;
  umask (0);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  while ((optc = getopt_long (argc, argv, "bcsDdg:m:o:pt:TvS:", long_options,
			      NULL)) != -1)
    {
      switch (optc)
	{
	case 'b':
	  make_backups = true;
	  if (optarg)
	    version_control_string = optarg;
	  break;
	case 'c':
	  break;
	case 's':
	  strip_files = true;
#ifdef SIGCHLD
	  /* System V fork+wait does not work if SIGCHLD is ignored.  */
	  signal (SIGCHLD, SIG_DFL);
#endif
	  break;
	case 'd':
	  dir_arg = true;
	  break;
	case 'D':
	  mkdir_and_install = true;
	  break;
	case 'v':
	  x.verbose = true;
	  break;
	case 'g':
	  group_name = optarg;
	  break;
	case 'm':
	  specified_mode = optarg;
	  break;
	case 'o':
	  owner_name = optarg;
	  break;
	case 'p':
	  x.preserve_timestamps = true;
	  break;
	case 'S':
	  make_backups = true;
	  backup_suffix_string = optarg;
	  break;
	case 't':
	  if (target_directory)
	    error (EXIT_FAILURE, 0,
		   _("multiple target directories specified"));
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
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  /* Check for invalid combinations of arguments. */
  if (dir_arg & strip_files)
    error (EXIT_FAILURE, 0,
	   _("the strip option may not be used when installing a directory"));
  if (dir_arg && target_directory)
    error (EXIT_FAILURE, 0,
	   _("target directory not allowed when installing a directory"));

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  x.backup_type = (make_backups
		   ? xget_version (_("backup type"),
				   version_control_string)
		   : no_backups);

  n_files = argc - optind;
  file = argv + optind;

  if (n_files <= ! (dir_arg || target_directory))
    {
      if (n_files <= 0)
	error (0, 0, _("missing file operand"));
      else
	error (0, 0, _("missing destination file operand after %s"),
	       quote (file[0]));
      usage (EXIT_FAILURE);
    }

  if (no_target_directory)
    {
      if (target_directory)
	error (EXIT_FAILURE, 0,
	       _("Cannot combine --target-directory (-t) "
		 "and --no-target-directory (-T)"));
      if (2 < n_files)
	{
	  error (0, 0, _("extra operand %s"), quote (file[2]));
	  usage (EXIT_FAILURE);
	}
    }
  else if (! (dir_arg || target_directory))
    {
      if (2 <= n_files && target_directory_operand (file[n_files - 1]))
	target_directory = file[--n_files];
      else if (2 < n_files)
	error (EXIT_FAILURE, 0, _("target %s is not a directory"),
	       quote (file[n_files - 1]));
    }

  if (specified_mode)
    {
      struct mode_change *change = mode_compile (specified_mode);
      if (!change)
	error (EXIT_FAILURE, 0, _("invalid mode %s"), quote (specified_mode));
      mode = mode_adjust (0, false, 0, change, NULL);
      dir_mode = mode_adjust (0, true, 0, change, &dir_mode_bits);
      free (change);
    }

  get_ids ();

  if (dir_arg)
    exit_status = savewd_process_files (n_files, file, process_dir, &x);
  else
    {
      /* FIXME: it's a little gross that this initialization is
	 required by copy.c::copy. */
      hash_init ();

      if (!target_directory)
        {
          if (! (mkdir_and_install
		 ? install_file_in_file_parents (file[0], file[1], &x)
		 : install_file_in_file (file[0], file[1], &x)))
	    exit_status = EXIT_FAILURE;
	}
      else
	{
	  int i;
	  dest_info_init (&x);
	  for (i = 0; i < n_files; i++)
	    if (! install_file_in_dir (file[i], target_directory, &x))
	      exit_status = EXIT_FAILURE;
	}
    }

  exit (exit_status);
}

/* Copy file FROM onto file TO, creating any missing parent directories of TO.
   Return true if successful.  */

static bool
install_file_in_file_parents (char const *from, char *to,
			      struct cp_options *x)
{
  bool save_working_directory =
    ! (IS_ABSOLUTE_FILE_NAME (from) && IS_ABSOLUTE_FILE_NAME (to));
  int status = EXIT_SUCCESS;

  struct savewd wd;
  savewd_init (&wd);
  if (! save_working_directory)
    savewd_finish (&wd);

  if (mkancesdirs (to, &wd, make_ancestor, x) == -1)
    {
      error (0, errno, _("cannot create directory %s"), to);
      status = EXIT_FAILURE;
    }

  if (save_working_directory)
    {
      int restore_result = savewd_restore (&wd, status);
      int restore_errno = errno;
      savewd_finish (&wd);
      if (EXIT_SUCCESS < restore_result)
	return false;
      if (restore_result < 0 && status == EXIT_SUCCESS)
	{
	  error (0, restore_errno, _("cannot create directory %s"), to);
	  return false;
	}
    }

  return (status == EXIT_SUCCESS && install_file_in_file (from, to, x));
}

/* Copy file FROM onto file TO and give TO the appropriate
   attributes.
   Return true if successful.  */

static bool
install_file_in_file (const char *from, const char *to,
		      const struct cp_options *x)
{
  struct stat from_sb;
  if (x->preserve_timestamps && stat (from, &from_sb) != 0)
    {
      error (0, errno, _("cannot stat %s"), quote (from));
      return false;
    }
  if (! copy_file (from, to, x))
    return false;
  if (strip_files)
    strip (to);
  if (x->preserve_timestamps && (strip_files || ! S_ISREG (from_sb.st_mode))
      && ! change_timestamps (&from_sb, to))
    return false;
  return change_attributes (to);
}

/* Copy file FROM into directory TO_DIR, keeping its same name,
   and give the copy the appropriate attributes.
   Return true if successful.  */

static bool
install_file_in_dir (const char *from, const char *to_dir,
		     const struct cp_options *x)
{
  const char *from_base = last_component (from);
  char *to = file_name_concat (to_dir, from_base, NULL);
  bool ret = install_file_in_file (from, to, x);
  free (to);
  return ret;
}

/* Copy file FROM onto file TO, creating TO if necessary.
   Return true if successful.  */

static bool
copy_file (const char *from, const char *to, const struct cp_options *x)
{
  bool copy_into_self;

  /* Allow installing from non-regular files like /dev/null.
     Charles Karney reported that some Sun version of install allows that
     and that sendmail's installation process relies on the behavior.
     However, since !x->recursive, the call to "copy" will fail if FROM
     is a directory.  */

  return copy (from, to, false, x, &copy_into_self, NULL);
}

/* Set the attributes of file or directory NAME.
   Return true if successful.  */

static bool
change_attributes (char const *name)
{
  /* chown must precede chmod because on some systems,
     chown clears the set[ug]id bits for non-superusers,
     resulting in incorrect permissions.
     On System V, users can give away files with chown and then not
     be able to chmod them.  So don't give files away.

     We don't normally ignore errors from chown because the idea of
     the install command is that the file is supposed to end up with
     precisely the attributes that the user specified (or defaulted).
     If the file doesn't end up with the group they asked for, they'll
     want to know.  */

  if (! (owner_id == (uid_t) -1 && group_id == (gid_t) -1)
      && chown (name, owner_id, group_id) != 0)
    error (0, errno, _("cannot change ownership of %s"), quote (name));
  else if (chmod (name, mode) != 0)
    error (0, errno, _("cannot change permissions of %s"), quote (name));
  else
    return true;

  return false;
}

/* Set the timestamps of file TO to match those of file FROM.
   Return true if successful.  */

static bool
change_timestamps (struct stat const *from_sb, char const *to)
{
  struct timespec timespec[2];
  timespec[0] = get_stat_atime (from_sb);
  timespec[1] = get_stat_mtime (from_sb);

  if (utimens (to, timespec))
    {
      error (0, errno, _("cannot set time stamps for %s"), quote (to));
      return false;
    }
  return true;
}

/* Strip the symbol table from the file NAME.
   We could dig the magic number out of the file first to
   determine whether to strip it, but the header files and
   magic numbers vary so much from system to system that making
   it portable would be very difficult.  Not worth the effort. */

static void
strip (char const *name)
{
  int status;
  pid_t pid = fork ();

  switch (pid)
    {
    case -1:
      error (EXIT_FAILURE, errno, _("fork system call failed"));
      break;
    case 0:			/* Child. */
      execlp ("strip", "strip", name, NULL);
      error (EXIT_FAILURE, errno, _("cannot run strip"));
      break;
    default:			/* Parent. */
      if (waitpid (pid, &status, 0) < 0)
	error (EXIT_FAILURE, errno, _("waiting for strip"));
      else if (! WIFEXITED (status) || WEXITSTATUS (status))
	error (EXIT_FAILURE, 0, _("strip process terminated abnormally"));
      break;
    }
}

/* Initialize the user and group ownership of the files to install. */

static void
get_ids (void)
{
  struct passwd *pw;
  struct group *gr;

  if (owner_name)
    {
      pw = getpwnam (owner_name);
      if (pw == NULL)
	{
	  unsigned long int tmp;
	  if (xstrtoul (owner_name, NULL, 0, &tmp, NULL) != LONGINT_OK
	      || UID_T_MAX < tmp)
	    error (EXIT_FAILURE, 0, _("invalid user %s"), quote (owner_name));
	  owner_id = tmp;
	}
      else
	owner_id = pw->pw_uid;
      endpwent ();
    }
  else
    owner_id = (uid_t) -1;

  if (group_name)
    {
      gr = getgrnam (group_name);
      if (gr == NULL)
	{
	  unsigned long int tmp;
	  if (xstrtoul (group_name, NULL, 0, &tmp, NULL) != LONGINT_OK
	      || GID_T_MAX < tmp)
	    error (EXIT_FAILURE, 0, _("invalid group %s"), quote (group_name));
	  group_id = tmp;
	}
      else
	group_id = gr->gr_gid;
      endgrent ();
    }
  else
    group_id = (gid_t) -1;
}

/* Report that directory DIR was made, if OPTIONS requests this.  */
static void
announce_mkdir (char const *dir, void *options)
{
  struct cp_options const *x = options;
  if (x->verbose)
    error (0, 0, _("creating directory %s"), quote (dir));
}

/* Make ancestor directory DIR, whose last file name component is
   COMPONENT, with options OPTIONS.  Assume the working directory is
   COMPONENT's parent.  */
static int
make_ancestor (char const *dir, char const *component, void *options)
{
  int r = mkdir (component, DEFAULT_MODE);
  if (r == 0)
    announce_mkdir (dir, options);
  return r;
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
Usage: %s [OPTION]... [-T] SOURCE DEST\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY\n\
  or:  %s [OPTION]... -t DIRECTORY SOURCE...\n\
  or:  %s [OPTION]... -d DIRECTORY...\n\
"),
	      program_name, program_name, program_name, program_name);
      fputs (_("\
In the first three forms, copy SOURCE to DEST or multiple SOURCE(s) to\n\
the existing DIRECTORY, while setting permission modes and owner/group.\n\
In the 4th form, create all components of the given DIRECTORY(ies).\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
      --backup[=CONTROL]  make a backup of each existing destination file\n\
  -b                  like --backup but does not accept an argument\n\
  -c                  (ignored)\n\
  -d, --directory     treat all arguments as directory names; create all\n\
                        components of the specified directories\n\
"), stdout);
      fputs (_("\
  -D                  create all leading components of DEST except the last,\n\
                        then copy SOURCE to DEST\n\
  -g, --group=GROUP   set group ownership, instead of process' current group\n\
  -m, --mode=MODE     set permission mode (as in chmod), instead of rwxr-xr-x\n\
  -o, --owner=OWNER   set ownership (super-user only)\n\
"), stdout);
      fputs (_("\
  -p, --preserve-timestamps   apply access/modification times of SOURCE files\n\
                        to corresponding destination files\n\
  -s, --strip         strip symbol tables\n\
  -S, --suffix=SUFFIX  override the usual backup suffix\n\
  -t, --target-directory=DIRECTORY  copy all SOURCE arguments into DIRECTORY\n\
  -T, --no-target-directory  treat DEST as a normal file\n\
  -v, --verbose       print the name of each directory as it is created\n\
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
