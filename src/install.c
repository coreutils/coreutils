/* install - copy files and set attributes
   Copyright (C) 89, 90, 91, 95, 96, 97, 1998 Free Software Foundation, Inc.

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

/* Copy files and set their permission modes and, if possible,
   their owner and group.  Used similarly to `cp'; typically
   used in Makefiles to copy programs into their destination
   directories.  It can also be used to create the destination
   directories and any leading directories, and to set the final
   directory's modes.  It refuses to copy files onto themselves.

   Options:
   -g, --group=GROUP
	Set the group ownership of the installed file or directory
	to the group ID of GROUP (default is process's current
	group).  GROUP may also be a numeric group ID.

   -m, --mode=MODE
	Set the permission mode for the installed file or directory
	to MODE, which is an octal number (default is 0755).

   -o, --owner=OWNER
	If run as root, set the ownership of the installed file to
	the user ID of OWNER (default is root).  OWNER may also be
	a numeric user ID.

   -c	No effect.  For compatibility with old Unix versions of install.

   -s, --strip
	Strip the symbol tables from installed files.

   -p, --preserve-timestamps
        Retain creation and modification timestamps when installing files.

   -d, --directory
	Create a directory and its leading directories, if they
	do not already exist.  Set the owner, group and mode
	as given on the command line.  Any leading directories
	that are created are also given those attributes.
	This is different from the SunOS 4.0 install, which gives
	directories that it creates the default attributes.

  -D
	Like the -d option, but a file is installed, along with the directory.
	Useful when installing into a new directory, and the install
	process doesn't properly comprehend making directories.

   David MacKenzie <djm@gnu.ai.mit.edu> */

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
#include "modechange.h"
#include "makepath.h"
#include "closeout.h"
#include "error.h"
#include "xstrtol.h"
#include "path-concat.h"
#include "cp-hash.h"
#include "copy.h"

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if HAVE_VALUES_H
# include <values.h>
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

/* True if C is an ASCII octal digit. */
#define isodigit(c) ((c) >= '0' && c <= '7')

/* Number of bytes of a file to copy at a time. */
#define READ_SIZE (32 * 1024)

#ifndef UID_T_MAX
# define UID_T_MAX TYPE_MAXIMUM (uid_t)
#endif

#ifndef GID_T_MAX
# define GID_T_MAX TYPE_MAXIMUM (gid_t)
#endif

char *base_name ();
char *dirname ();
int full_write ();
int isdir ();
enum backup_type get_version ();

int stat ();

static int change_timestamps PARAMS ((const char *from, const char *to));
static int change_attributes PARAMS ((const char *path, mode_t mode));
static int copy_file PARAMS ((const char *from, const char *to,
			      const struct cp_options *x));
static int install_file_to_path PARAMS ((const char *from, const char *to,
					 const struct cp_options *x));
static int install_file_in_dir PARAMS ((const char *from, const char *to_dir,
					const struct cp_options *x));
static int install_file_in_file PARAMS ((const char *from, const char *to,
					 const struct cp_options *x));
static void get_ids PARAMS ((void));
static void strip PARAMS ((const char *path));
static void usage PARAMS ((int status));

/* The name this program was run with, for error messages. */
char *program_name;

/* FIXME: document */
enum backup_type backup_type;

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

/* If nonzero, strip executable files after copying them. */
static int strip_files;

/* If nonzero, install a directory instead of a regular file. */
static int dir_arg;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"strip", no_argument, NULL, 's'},
  {"directory", no_argument, NULL, 'd'},
  {"group", required_argument, NULL, 'g'},
  {"mode", required_argument, NULL, 'm'},
  {"owner", required_argument, NULL, 'o'},
  {"preserve-timestamps", no_argument, NULL, 'p'},
  {"backup", no_argument, NULL, 'b'},
  {"version-control", required_argument, NULL, 'V'},
  {"verbose", no_argument, NULL, 'v'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = 1;
  x->dereference = 1;
  x->force = 1;

  /* If unlink fails, try to proceed anyway.  */
  x->failed_unlink_is_fatal = 0;

  x->hard_link = 0;
  x->interactive = 0;
  x->move_mode = 0;
  x->myeuid = geteuid ();
  x->one_file_system = 0;
  x->preserve_owner_and_group = 0;
  x->preserve_chmod_bits = 0;
  x->preserve_timestamps = 0;
  x->require_preserve = 0;
  x->recursive = 0;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = 0;
  x->set_mode = 1;
  x->mode = 0755;
  x->umask_kill = 0;
  x->update = 0;
  x->verbose = 0;
  x->xstat = stat;
}

int
main (int argc, char **argv)
{
  int optc;
  int errors = 0;
  char *symbolic_mode = NULL;
  int make_backups = 0;
  char *version;
  int mkdir_and_install = 0;
  struct cp_options x;
  int n_files;
  char **file;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  cp_option_init (&x);

  owner_name = NULL;
  group_name = NULL;
  strip_files = 0;
  dir_arg = 0;
  umask (0);

  version = getenv ("SIMPLE_BACKUP_SUFFIX");
  if (version)
    simple_backup_suffix = version;
  version = getenv ("VERSION_CONTROL");

  while ((optc = getopt_long (argc, argv, "bcsDdg:m:o:pvV:S:", long_options,
			      NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'b':
	  make_backups = 1;
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
	  x.verbose = 0;
	  break;
	case 'g':
	  group_name = optarg;
	  break;
	case 'm':
	  symbolic_mode = optarg;
	  break;
	case 'o':
	  owner_name = optarg;
	  break;
	case 'p':
	  x.preserve_timestamps = 1;
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
      printf ("install (%s) %s\n", GNU_PACKAGE, VERSION);
      close_stdout ();
      exit (0);
    }

  if (show_help)
    usage (0);

  /* Check for invalid combinations of arguments. */
  if (dir_arg && strip_files)
    error (1, 0,
	   _("the strip option may not be used when installing a directory"));

  x.backup_type = (make_backups ? get_version (version) : none);

  n_files = argc - optind;
  file = argv + optind;

  if (n_files == 0 || (n_files == 1 && !dir_arg))
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  if (symbolic_mode)
    {
      struct mode_change *change = mode_compile (symbolic_mode, 0);
      if (change == MODE_INVALID)
	error (1, 0, _("invalid mode `%s'"), symbolic_mode);
      else if (change == MODE_MEMORY_EXHAUSTED)
	error (1, 0, _("virtual memory exhausted"));
      x.mode = mode_adjust (0, change);
    }

  get_ids ();

  if (dir_arg)
    {
      int i;
      for (i = 0; i < n_files; i++)
	{
	  errors |=
	    make_path (file[i], x.mode, x.mode, owner_id, group_id, 0,
		       (x.verbose ? "creating directory `%s'" : NULL));
	}
    }
  else
    {
      /* FIXME: it's a little gross that this initialization is
	 required by copy.c::copy. */
      hash_init (INITIAL_HASH_MODULE, INITIAL_ENTRY_TAB_SIZE);

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
		     _("installing multiple files, but last argument (%s) \
is not a directory"),
		     dest);
	      usage (1);
	    }
	  for (i = 0; i < n_files - 1; i++)
	    {
	      errors |= install_file_in_dir (file[i], dest, &x);
	    }
	}
    }

  if (x.verbose)
    close_stdout ();
  exit (errors);
}

/* Copy file FROM onto file TO, creating any missing parent directories of TO.
   Return 0 if successful, 1 if an error occurs */

static int
install_file_to_path (const char *from, const char *to,
		      const struct cp_options *x)
{
  char *dest_dir;
  int fail;

  dest_dir = dirname (to);

  /* check to make sure this is a path (not install a b ) */
  if (!STREQ (dest_dir, ".")
      && !isdir (dest_dir))
    {
      /* FIXME: Note that it's a little kludgey (even dangerous) that we
	 derive the permissions for parent directories from the permissions
	 specfied for the file, but since this option is intended mainly to
	 help installers when the distribution doesn't provide proper install
	 rules, it's not so bad.  Maybe use something like this instead:
	 int parent_dir_mode = (mode | (S_IRUGO | S_IXUGO)) & (~SPECIAL_BITS);
	 */
      fail = make_path (dest_dir, x->mode, x->mode, owner_id, group_id, 0,
			(x->verbose ? _("creating directory `%s'") : NULL));
    }
  else
    {
      fail = install_file_in_file (from, to, x);
    }

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
  if (change_attributes (to, x->mode))
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
  char *from_base;
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
      error (0, 0, _("`%s' is a directory"), from);
      return 1;
    }

  fail = copy (from, to, nonexistent_dst, x, &copy_into_self, NULL);

  return fail;
}

/* Set the attributes of file or directory PATH.
   Return 0 if successful, 1 if not. */

static int
change_attributes (const char *path, mode_t mode)
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
    err = errno;
  if (chmod (path, mode))
    err = errno;
  if (err)
    {
      error (0, err, "%s", path);
      return 1;
    }
  return 0;
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
      error (0, errno, "%s", from);
      return 1;
    }
  utb.actime = stb.st_atime;
  utb.modtime = stb.st_mtime;
  if (utime (to, &utb))
    {
      error (0, errno, "%s", to);
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
  int pid, status;

  pid = fork ();
  switch (pid)
    {
    case -1:
      error (1, errno, _("fork system call failed"));
      break;
    case 0:			/* Child. */
      execlp ("strip", "strip", path, NULL);
      error (1, errno, _("cannot run strip"));
      break;
    default:			/* Parent. */
      /* Parent process. */
      while (pid != wait (&status))	/* Wait for kid to finish. */
	/* Do nothing. */ ;
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
	  long int tmp_long;
	  if (xstrtol (owner_name, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long < 0 || tmp_long > UID_T_MAX)
	    error (1, 0, _("invalid user `%s'"), owner_name);
	  owner_id = (uid_t) tmp_long;
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
	  long int tmp_long;
	  if (xstrtol (group_name, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long < 0 || tmp_long > GID_T_MAX)
	    error (1, 0, _("invalid group `%s'"), group_name);
	  group_id = (gid_t) tmp_long;
	}
      else
	group_id = gr->gr_gid;
      endgrent ();
    }
  else
    group_id = (gid_t) -1;
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
Usage: %s [OPTION]... SOURCE DEST           (1st format)\n\
  or:  %s [OPTION]... SOURCE... DIRECTORY   (2nd format)\n\
  or:  %s -d [OPTION]... DIRECTORY...       (3rd format)\n\
"),
	      program_name, program_name, program_name);
      printf (_("\
In the first two formats, copy SOURCE to DEST or multiple SOURCE(s) to\n\
the existing DIRECTORY, while setting permission modes and owner/group.\n\
In the third format, create all components of the given DIRECTORY(ies).\n\
\n\
  -b, --backup        make backup before removal\n\
  -c                  (ignored)\n\
  -d, --directory     treat all arguments as directory names; create all\n\
                        components of the specified directories\n\
  -D                  create all leading components of DEST except the last,\n\
                        then copy SOURCE to DEST;  useful in the 1st format\n\
  -g, --group=GROUP   set group ownership, instead of process' current group\n\
  -m, --mode=MODE     set permission mode (as in chmod), instead of rwxr-xr-x\n\
  -o, --owner=OWNER   set ownership (super-user only)\n\
  -p, --preserve-timestamps   apply access/modification times of SOURCE files\n\
                        to corresponding destination files\n\
  -s, --strip         strip symbol tables, only for 1st and 2nd formats\n\
  -S, --suffix=SUFFIX override the usual backup suffix\n\
      --verbose       print the name of each directory as it is created\n\
  -V, --version-control=WORD   override the usual version control\n\
      --help          display this help and exit\n\
      --version       output version information and exit\n\
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
