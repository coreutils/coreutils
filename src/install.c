/* install - copy files and set attributes
   Copyright (C) 89, 90, 91, 95, 96, 1997 Free Software Foundation, Inc.

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
#include "error.h"
#include "xstrtol.h"

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if HAVE_VALUES_H
# include <values.h>
#endif

#ifndef BITSPERBYTE
# define BITSPERBYTE 8
#endif

struct passwd *getpwnam ();
struct group *getgrnam ();

#ifndef _POSIX_VERSION
uid_t getuid ();
gid_t getgid ();
#endif

#ifndef HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#ifndef HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

/* True if C is an ASCII octal digit. */
#define isodigit(c) ((c) >= '0' && c <= '7')

/* Number of bytes of a file to copy at a time. */
#define READ_SIZE (32 * 1024)

#ifndef UID_T_MAX
# define UID_T_MAX ((uid_t)(~((unsigned long)1 << ((sizeof (uid_t) \
						    * BITSPERBYTE - 1)))))
#endif

#ifndef GID_T_MAX
# define GID_T_MAX ((gid_t)(~((unsigned long)1 << ((sizeof (gid_t) \
						    * BITSPERBYTE - 1)))))
#endif

char *base_name ();
char *stpcpy ();
char *xmalloc ();
int safe_read ();
int full_write ();
int isdir ();
enum backup_type get_version ();

static int change_timestamps __P ((const char *from, const char *to));
static int change_attributes __P ((const char *path, int no_need_to_chown));
static int copy_file __P ((const char *from, const char *to, int *to_created));
static int install_file_in_dir __P ((const char *from, const char *to_dir));
static int install_file_in_file __P ((const char *from, const char *to));
static void get_ids __P ((void));
static void strip __P ((const char *path));
static void usage __P ((int status));

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
static int mode;

/* If nonzero, strip executable files after copying them. */
static int strip_files;

/* If nonzero, preserve timestamps when installing files. */
static int preserve_timestamps;

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
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
  int optc;
  int errors = 0;
  char *symbolic_mode = NULL;
  int make_backups = 0;
  char *version;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  owner_name = NULL;
  group_name = NULL;
  mode = 0755;
  strip_files = 0;
  preserve_timestamps = 0;
  dir_arg = 0;
  umask (0);

   version = getenv ("SIMPLE_BACKUP_SUFFIX");
   if (version)
      simple_backup_suffix = version;
   version = getenv ("VERSION_CONTROL");

  while ((optc = getopt_long (argc, argv, "bcsdg:m:o:pV:S:", long_options,
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
	  preserve_timestamps = 1;
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
      exit (0);
    }

  if (show_help)
    usage (0);

  /* Check for invalid combinations of arguments. */
  if (dir_arg && strip_files)
    error (1, 0,
	   _("the strip option may not be used when installing a directory"));

  if (make_backups)
    backup_type = get_version (version);

  if (optind == argc || (optind == argc - 1 && !dir_arg))
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
      mode = mode_adjust (0, change);
    }

  get_ids ();

  if (dir_arg)
    {
      for (; optind < argc; ++optind)
	{
	  errors |=
	    make_path (argv[optind], mode, mode, owner_id, group_id, 0, NULL);
	}
    }
  else
    {
      if (optind == argc - 2)
	{
	  if (!isdir (argv[argc - 1]))
	    errors = install_file_in_file (argv[argc - 2], argv[argc - 1]);
	  else
	    errors = install_file_in_dir (argv[argc - 2], argv[argc - 1]);
	}
      else
	{
	  if (!isdir (argv[argc - 1]))
	    usage (1);
	  for (; optind < argc - 1; ++optind)
	    {
	      errors |= install_file_in_dir (argv[optind], argv[argc - 1]);
	    }
	}
    }

  exit (errors);
}

/* Copy file FROM onto file TO and give TO the appropriate
   attributes.
   Return 0 if successful, 1 if an error occurs. */

static int
install_file_in_file (const char *from, const char *to)
{
  int to_created;
  int no_need_to_chown;

  if (copy_file (from, to, &to_created))
    return 1;
  if (strip_files)
    strip (to);
  no_need_to_chown = (to_created
		      && owner_name == NULL
		      && group_name == NULL);
  if (change_attributes (to, no_need_to_chown))
    return 1;
  if (preserve_timestamps)
    return change_timestamps (from, to);
  return 0;
}

/* Copy file FROM into directory TO_DIR, keeping its same name,
   and give the copy the appropriate attributes.
   Return 0 if successful, 1 if not. */

static int
install_file_in_dir (const char *from, const char *to_dir)
{
  char *from_base;
  char *to;
  int ret;

  from_base = base_name (from);
  to = xmalloc ((unsigned) (strlen (to_dir) + strlen (from_base) + 2));
  stpcpy (stpcpy (stpcpy (to, to_dir), "/"), from_base);
  ret = install_file_in_file (from, to);
  free (to);
  return ret;
}

/* A chunk of a file being copied. */
static char buffer[READ_SIZE];

/* Copy file FROM onto file TO, creating TO if necessary.
   Return 0 if the copy is successful, 1 if not.  If the copy is
   successful, set *TO_CREATED to nonzero if TO was created (if it did
   not exist or did, but was unlinked) and to zero otherwise.  If the
   copy fails, don't modify *TO_CREATED.  */

static int
copy_file (const char *from, const char *to, int *to_created)
{
  int fromfd, tofd;
  int bytes;
  int ret = 0;
  struct stat from_stats, to_stats;
  int target_created = 1;

  if (stat (from, &from_stats))
    {
      error (0, errno, "%s", from);
      return 1;
    }

  /* Allow installing from non-regular files like /dev/null.
     Charles Karney reported that some Sun version of install allows that
     and that sendmail's installation process relies on the behavior.  */
  if (S_ISDIR (from_stats.st_mode))
    {
      error (0, 0, _("`%s' is a directory"), from);
      return 1;
    }
  if (stat (to, &to_stats) == 0)
    {
      if (!S_ISREG (to_stats.st_mode))
	{
	  error (0, 0, _("`%s' is not a regular file"), to);
	  return 1;
	}
      if (from_stats.st_dev == to_stats.st_dev
	  && from_stats.st_ino == to_stats.st_ino)
	{
	  error (0, 0, _("`%s' and `%s' are the same file"), from, to);
	  return 1;
	}

      /* The destination file exists.  Try to back it up if required.  */
      if (backup_type != none)
        {
	  char *tmp_backup = find_backup_file_name (to);
	  char *dst_backup;

	  if (tmp_backup == NULL)
	    error (1, 0, "virtual memory exhausted");
	  dst_backup = (char *) alloca (strlen (tmp_backup) + 1);
	  strcpy (dst_backup, tmp_backup);
	  free (tmp_backup);
	  if (rename (to, dst_backup))
	    {
	      if (errno != ENOENT)
		{
		  error (0, errno, "cannot backup `%s'", to);
		  return 1;
		}
	    }
	}

      /* If unlink fails, try to proceed anyway.  */
      if (unlink (to))
	target_created = 0;
    }

  fromfd = open (from, O_RDONLY, 0);
  if (fromfd == -1)
    {
      error (0, errno, "%s", from);
      return 1;
    }

  /* Make sure to open the file in a mode that allows writing. */
  tofd = open (to, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (tofd == -1)
    {
      error (0, errno, "%s", to);
      close (fromfd);
      return 1;
    }

  while ((bytes = safe_read (fromfd, buffer, READ_SIZE)) > 0)
    if (full_write (tofd, buffer, bytes) < 0)
      {
	error (0, errno, "%s", to);
	goto copy_error;
      }

  if (bytes == -1)
    {
      error (0, errno, "%s", from);
      goto copy_error;
    }

  if (close (fromfd) < 0)
    {
      error (0, errno, "%s", from);
      ret = 1;
    }
  if (close (tofd) < 0)
    {
      error (0, errno, "%s", to);
      ret = 1;
    }
  if (ret == 0)
    *to_created = target_created;
  return ret;

 copy_error:
  close (fromfd);
  close (tofd);
  return 1;
}

/* Set the attributes of file or directory PATH.
   If NO_NEED_TO_CHOWN is nonzero, don't call chown.
   Return 0 if successful, 1 if not. */

static int
change_attributes (const char *path, int no_need_to_chown)
{
  int err = 0;

  /* chown must precede chmod because on some systems,
     chown clears the set[ug]id bits for non-superusers,
     resulting in incorrect permissions.
     On System V, users can give away files with chown and then not
     be able to chmod them.  So don't give files away.

     We don't pass -1 to chown to mean "don't change the value"
     because SVR3 and earlier non-BSD systems don't support that.

     We don't normally ignore errors from chown because the idea of
     the install command is that the file is supposed to end up with
     precisely the attributes that the user specified (or defaulted).
     If the file doesn't end up with the group they asked for, they'll
     want to know.  But AFS returns EPERM when you try to change a
     file's group; thus the kludge.  */

  if (!no_need_to_chown && chown (path, owner_id, group_id)
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
    owner_id = getuid ();

  if (group_name)
    {
      gr = getgrnam (group_name);
      if (gr == NULL)
	{
	  long int tmp_long;
	  if (xstrtol (group_name, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long < 0 || tmp_long > (long) GID_T_MAX)
	    error (1, 0, _("invalid group `%s'"), group_name);
	  group_id = (gid_t) tmp_long;
	}
      else
	group_id = gr->gr_gid;
      endgrent ();
    }
  else
    group_id = getgid ();
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
In first two formats, copy SOURCE to DEST or multiple SOURCE(s) to\n\
DIRECTORY, while setting permission modes and owner/group.  In third\n\
format, make all components of the given DIRECTORY(ies).\n\
\n\
  -b, --backup        make backup before removal\n\
  -c                  (ignored)\n\
  -d, --directory     create [leading] directories, mandatory for 3rd format\n\
  -g, --group=GROUP   set group ownership, instead of process' current group\n\
  -m, --mode=MODE     set permission mode (as in chmod), instead of rwxr-xr-x\n\
  -o, --owner=OWNER   set ownership (super-user only)\n\
  -p, --preserve-timestamps   Retain previous creation/modification times\n\
  -s, --strip         strip symbol tables, only for 1st and 2nd formats\n\
  -S, --suffix=SUFFIX override the usual backup suffix\n\
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
      puts (_("\nReport bugs to <fileutils-bugs@gnu.ai.mit.edu>."));
    }
  exit (status);
}
