/* install - copy files and set attributes
   Copyright (C) 89, 90, 91, 1995-2002 Free Software Foundation, Inc.

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

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "system.h"
#include "backupfile.h"
#include "error.h"
#include "cp-hash.h"
#include "copy.h"
#include "dirname.h"
#include "makepath.h"
#include "modechange.h"
#include "path-concat.h"
#include "quote.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "install"

#define AUTHORS "David MacKenzie"

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

struct passwd *getpwnam ();
struct group *getgrnam ();

#ifndef _POSIX_VERSION
uid_t getuid ();
gid_t getgid ();
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

int isdir ();

int stat ();

static int change_timestamps (const char *from, const char *to);
static int change_attributes (const char *path);
static int copy_file (const char *from, const char *to,
		      const struct cp_options *x);
static int install_file_to_path (const char *from, const char *to,
				 const struct cp_options *x);
static int install_file_in_dir (const char *from, const char *to_dir,
				const struct cp_options *x);
static int install_file_in_file (const char *from, const char *to,
				 const struct cp_options *x);
static void get_ids (void);
static void strip (const char *path);
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

/* The permissions to which the files will be set.  The umask has
   no effect. */
static mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

/* If nonzero, strip executable files after copying them. */
static int strip_files;

/* If nonzero, install a directory instead of a regular file. */
static int dir_arg;

static struct option const long_options[] =
{
  {"backup", optional_argument, NULL, 'b'},
  {"directory", no_argument, NULL, 'd'},
  {"group", required_argument, NULL, 'g'},
  {"mode", required_argument, NULL, 'm'},
  {"owner", required_argument, NULL, 'o'},
  {"preserve-timestamps", no_argument, NULL, 'p'},
  {"strip", no_argument, NULL, 's'},
  {"suffix", required_argument, NULL, 'S'},
  {"version-control", required_argument, NULL, 'V'}, /* Deprecated. FIXME. */
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = 1;
  x->dereference = DEREF_ALWAYS;
  x->unlink_dest_before_opening = 1;
  x->unlink_dest_after_failed_open = 0;
  x->hard_link = 0;
  x->interactive = I_UNSPECIFIED;
  x->move_mode = 0;
  x->myeuid = geteuid ();
  x->one_file_system = 0;
  x->preserve_ownership = 0;
  x->preserve_links = 0;
  x->preserve_mode = 0;
  x->preserve_timestamps = 0;
  x->require_preserve = 0;
  x->recursive = 0;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = 0;
  x->backup_type = none;

  /* Create destination files initially writable so we can run strip on them.
     Although GNU strip works fine on read-only files, some others
     would fail.  */
  x->set_mode = 1;
  x->mode = S_IRUSR | S_IWUSR;
  x->stdin_tty = 0;

  x->umask_kill = 0;
  x->update = 0;
  x->verbose = 0;
  x->xstat = stat;
  x->dest_info = NULL;
  x->src_info = NULL;
}

int
main (int argc, char **argv)
{
  int optc;
  int errors = 0;
  const char *specified_mode = NULL;
  int make_backups = 0;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  int mkdir_and_install = 0;
  struct cp_options x;
  int n_files;
  char **file;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  cp_option_init (&x);

  owner_name = NULL;
  group_name = NULL;
  strip_files = 0;
  dir_arg = 0;
  umask (0);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  while ((optc = getopt_long (argc, argv, "bcsDdg:m:o:pvV:S:", long_options,
			      NULL)) != -1)
    {
      switch (optc)
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
	case 'c':
	  break;
	case 's':
	  strip_files = 1;
	  break;
	case 'd':
	  dir_arg = 1;
	  break;
	case 'D':
	  mkdir_and_install = 1;
	  break;
	case 'v':
	  x.verbose = 1;
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
	  x.preserve_timestamps = 1;
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

  /* Check for invalid combinations of arguments. */
  if (dir_arg && strip_files)
    error (EXIT_FAILURE, 0,
	   _("the strip option may not be used when installing a directory"));

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  x.backup_type = (make_backups
		   ? xget_version (_("backup type"),
				   version_control_string)
		   : none);

  n_files = argc - optind;
  file = argv + optind;

  if (n_files == 0 || (n_files == 1 && !dir_arg))
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  if (specified_mode)
    {
      struct mode_change *change = mode_compile (specified_mode, 0);
      if (change == MODE_INVALID)
	error (EXIT_FAILURE, 0, _("invalid mode %s"), quote (specified_mode));
      else if (change == MODE_MEMORY_EXHAUSTED)
	xalloc_die ();
      mode = mode_adjust (0, change);
    }

  get_ids ();

  if (dir_arg)
    {
      int i;
      for (i = 0; i < n_files; i++)
	{
	  errors |=
	    make_path (file[i], mode, mode, owner_id, group_id, 0,
		       (x.verbose ? _("creating directory %s") : NULL));
	}
    }
  else
    {
      /* FIXME: it's a little gross that this initialization is
	 required by copy.c::copy. */
      hash_init ();

      if (n_files == 2)
        {
          if (mkdir_and_install)
	    errors = install_file_to_path (file[0], file[1], &x);
	  else if (!isdir (file[1]))
	    errors = install_file_in_file (file[0], file[1], &x);
	  else
	    errors = install_file_in_dir (file[0], file[1], &x);
	}
      else
	{
	  int i;
	  const char *dest = file[n_files - 1];
	  if (!isdir (dest))
	    {
	      error (0, 0,
		     _("installing multiple files, but last argument, %s \
is not a directory"),
		     quote (dest));
	      usage (EXIT_FAILURE);
	    }

	  dest_info_init (&x);
	  for (i = 0; i < n_files - 1; i++)
	    {
	      errors |= install_file_in_dir (file[i], dest, &x);
	    }
	}
    }

  exit (errors);
}

/* Copy file FROM onto file TO, creating any missing parent directories of TO.
   Return 0 if successful, 1 if an error occurs */

static int
install_file_to_path (const char *from, const char *to,
		      const struct cp_options *x)
{
  char *dest_dir;
  int fail = 0;

  dest_dir = dir_name (to);

  /* check to make sure this is a path (not install a b ) */
  if (!STREQ (dest_dir, ".")
      && !isdir (dest_dir))
    {
      /* Someone will probably ask for a new option or three to specify
	 owner, group, and permissions for parent directories.  Remember
	 that this option is intended mainly to help installers when the
	 distribution doesn't provide proper install rules.  */
#define DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
      fail = make_path (dest_dir, DIR_MODE, DIR_MODE, owner_id, group_id, 0,
			(x->verbose ? _("creating directory %s") : NULL));
    }

  if (fail == 0)
    fail = install_file_in_file (from, to, x);

  free (dest_dir);

  return fail;
}

/* Copy file FROM onto file TO and give TO the appropriate
   attributes.
   Return 0 if successful, 1 if an error occurs. */

static int
install_file_in_file (const char *from, const char *to,
		      const struct cp_options *x)
{
  if (copy_file (from, to, x))
    return 1;
  if (strip_files)
    strip (to);
  if (change_attributes (to))
    return 1;
  if (x->preserve_timestamps)
    return change_timestamps (from, to);
  return 0;
}

/* Copy file FROM into directory TO_DIR, keeping its same name,
   and give the copy the appropriate attributes.
   Return 0 if successful, 1 if not. */

static int
install_file_in_dir (const char *from, const char *to_dir,
		     const struct cp_options *x)
{
  const char *from_base;
  char *to;
  int ret;

  from_base = base_name (from);
  to = path_concat (to_dir, from_base, NULL);
  ret = install_file_in_file (from, to, x);
  free (to);
  return ret;
}

/* Copy file FROM onto file TO, creating TO if necessary.
   Return 0 if the copy is successful, 1 if not.  */

static int
copy_file (const char *from, const char *to, const struct cp_options *x)
{
  int fail;
  int nonexistent_dst = 0;
  int copy_into_self;

  /* Allow installing from non-regular files like /dev/null.
     Charles Karney reported that some Sun version of install allows that
     and that sendmail's installation process relies on the behavior.  */
  if (isdir (from))
    {
      error (0, 0, _("%s is a directory"), quote (from));
      return 1;
    }

  fail = copy (from, to, nonexistent_dst, x, &copy_into_self, NULL);

  return fail;
}

/* Set the attributes of file or directory PATH.
   Return 0 if successful, 1 if not. */

static int
change_attributes (const char *path)
{
  int err = 0;

  /* chown must precede chmod because on some systems,
     chown clears the set[ug]id bits for non-superusers,
     resulting in incorrect permissions.
     On System V, users can give away files with chown and then not
     be able to chmod them.  So don't give files away.

     We don't normally ignore errors from chown because the idea of
     the install command is that the file is supposed to end up with
     precisely the attributes that the user specified (or defaulted).
     If the file doesn't end up with the group they asked for, they'll
     want to know.  But AFS returns EPERM when you try to change a
     file's group; thus the kludge.  */

  if (chown (path, owner_id, group_id)
#ifdef AFS
      && errno != EPERM
#endif
      )
    {
      error (0, errno, "cannot change ownership of %s", quote (path));
      err = 1;
    }

  if (!err && chmod (path, mode))
    {
      error (0, errno, "cannot change permissions of %s", quote (path));
      err = 1;
    }

  return err;
}

/* Set the timestamps of file TO to match those of file FROM.
   Return 0 if successful, 1 if not. */

static int
change_timestamps (const char *from, const char *to)
{
  struct stat stb;
  struct utimbuf utb;

  if (stat (from, &stb))
    {
      error (0, errno, _("cannot obtain time stamps for %s"), quote (from));
      return 1;
    }

  /* There's currently no interface to set file timestamps with
     better than 1-second resolution, so discard any fractional
     part of the source timestamp.  */

  utb.actime = stb.st_atime;
  utb.modtime = stb.st_mtime;
  if (utime (to, &utb))
    {
      error (0, errno, _("cannot set time stamps for %s"), quote (to));
      return 1;
    }
  return 0;
}

/* Strip the symbol table from the file PATH.
   We could dig the magic number out of the file first to
   determine whether to strip it, but the header files and
   magic numbers vary so much from system to system that making
   it portable would be very difficult.  Not worth the effort. */

static void
strip (const char *path)
{
  int status;
  pid_t pid = fork ();

  switch (pid)
    {
    case -1:
      error (EXIT_FAILURE, errno, _("fork system call failed"));
      break;
    case 0:			/* Child. */
      execlp ("strip", "strip", path, NULL);
      error (EXIT_FAILURE, errno, _("cannot run strip"));
      break;
    default:			/* Parent. */
      /* Parent process. */
      while (pid != wait (&status))	/* Wait for kid to finish. */
	/* Do nothing. */ ;
      if (status)
	error (EXIT_FAILURE, 0, _("strip failed"));
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

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... SOURCE DEST           (1st format)\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY   (2nd format)\n\
  or:  %s -d [OPTION]... DIRECTORY...       (3rd format)\n\
"),
	      program_name, program_name, program_name);
      fputs (_("\
In the first two formats, copy SOURCE to DEST or multiple SOURCE(s) to\n\
the existing DIRECTORY, while setting permission modes and owner/group.\n\
In the third format, create all components of the given DIRECTORY(ies).\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
      --backup[=CONTROL] make a backup of each existing destination file\n\
  -b                  like --backup but does not accept an argument\n\
  -c                  (ignored)\n\
  -d, --directory     treat all arguments as directory names; create all\n\
                        components of the specified directories\n\
"), stdout);
      fputs (_("\
  -D                  create all leading components of DEST except the last,\n\
                        then copy SOURCE to DEST;  useful in the 1st format\n\
  -g, --group=GROUP   set group ownership, instead of process' current group\n\
  -m, --mode=MODE     set permission mode (as in chmod), instead of rwxr-xr-x\n\
  -o, --owner=OWNER   set ownership (super-user only)\n\
"), stdout);
      fputs (_("\
  -p, --preserve-timestamps   apply access/modification times of SOURCE files\n\
                        to corresponding destination files\n\
  -s, --strip         strip symbol tables, only for 1st and 2nd formats\n\
  -S, --suffix=SUFFIX override the usual backup suffix\n\
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
