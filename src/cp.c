/* cp.c  -- file copying (main routines)
   Copyright (C) 89, 90, 91, 95, 96, 97, 1998 Free Software Foundation.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Torbjorn Granlund, David MacKenzie, and Jim Meyering. */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <getopt.h>

#include "system.h"
#include "backupfile.h"
#include "argmatch.h"
#include "path-concat.h"
#include "closeout.h"
#include "cp-hash.h"
#include "copy.h"
#include "error.h"
#include "dirname.h"

#ifndef _POSIX_VERSION
uid_t geteuid ();
#endif

/* Used by do_copy, make_path_private, and re_protect
   to keep a list of leading directories whose protections
   need to be fixed after copying. */
struct dir_attr
{
  int is_new_dir;
  int slash_offset;
  struct dir_attr *next;
};

int stat ();
int lstat ();

char *base_name ();
enum backup_type get_version ();
void strip_trailing_slashes ();
char *xstrdup ();

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

/* The invocation name of this program.  */
char *program_name;

/* If nonzero, the command "cp x/e_file e_dir" uses "e_dir/x/e_file"
   as its destination instead of the usual "e_dir/e_file." */
static int flag_path = 0;

static char const *const sparse_type_string[] =
{
  "never", "auto", "always", 0
};

static enum Sparse_type const sparse_type[] =
{
  SPARSE_NEVER, SPARSE_AUTO, SPARSE_ALWAYS
};

/* The error code to return to the system. */
static int exit_status = 0;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_opts[] =
{
  {"archive", no_argument, NULL, 'a'},
  {"backup", no_argument, NULL, 'b'},
  {"force", no_argument, NULL, 'f'},
  {"sparse", required_argument, NULL, CHAR_MAX + 1},
  {"interactive", no_argument, NULL, 'i'},
  {"link", no_argument, NULL, 'l'},
  {"no-dereference", no_argument, NULL, 'd'},
  {"one-file-system", no_argument, NULL, 'x'},
  {"parents", no_argument, NULL, 'P'},
  {"path", no_argument, NULL, 'P'},
  {"preserve", no_argument, NULL, 'p'},
  {"recursive", no_argument, NULL, 'R'},
  {"suffix", required_argument, NULL, 'S'},
  {"symbolic-link", no_argument, NULL, 's'},
  {"update", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {"version-control", required_argument, NULL, 'V'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

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
Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n\
\n\
  -a, --archive                same as -dpR\n\
  -b, --backup                 make backup before removal\n\
  -d, --no-dereference         preserve links\n\
  -f, --force                  remove existing destinations, never prompt\n\
  -i, --interactive            prompt before overwrite\n\
  -l, --link                   link files instead of copying\n\
  -p, --preserve               preserve file attributes if possible\n\
  -P, --parents                append source path to DIRECTORY\n\
  -r                           copy recursively, non-directories as files\n\
      --sparse=WHEN            control creation of sparse files\n\
  -R, --recursive              copy directories recursively\n\
  -s, --symbolic-link          make symbolic links instead of copying\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
  -u, --update                 copy only when the SOURCE file is newer\n\
                                 than the destination file or when the\n\
                                 destination file is missing\n\
  -v, --verbose                explain what is being done\n\
  -V, --version-control=WORD   override the usual version control\n\
  -x, --one-file-system        stay on this file system\n\
      --help                   display this help and exit\n\
      --version                output version information and exit\n\
\n\
By default, sparse SOURCE files are detected by a crude heuristic and the\n\
corresponding DEST file is made sparse as well.  That is the behavior\n\
selected by --sparse=auto.  Specify --sparse=always to create a sparse DEST\n\
file whenever the SOURCE file contains a long enough sequence of zero bytes.\n\
Use --sparse=never to inhibit creation of sparse files.\n\
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
      printf (_("\
\n\
As a special case, cp makes a backup of SOURCE when the force and backup\n\
options are given and SOURCE and DEST are the same name for an existing,\n\
regular file.\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
      close_stdout ();
    }
  exit (status);
}

/* Ensure that the parent directories of CONST_DST_PATH have the
   correct protections, for the --parents option.  This is done
   after all copying has been completed, to allow permissions
   that don't include user write/execute.

   SRC_OFFSET is the index in CONST_DST_PATH of the beginning of the
   source directory name.

   ATTR_LIST is a null-terminated linked list of structures that
   indicates the end of the filename of each intermediate directory
   in CONST_DST_PATH that may need to have its attributes changed.
   The command `cp --parents --preserve a/b/c d/e_dir' changes the
   attributes of the directories d/e_dir/a and d/e_dir/a/b to match
   the corresponding source directories regardless of whether they
   existed before the `cp' command was given.

   Return 0 if the parent of CONST_DST_PATH and any intermediate
   directories specified by ATTR_LIST have the proper permissions
   when done, otherwise 1. */

static int
re_protect (const char *const_dst_path, int src_offset,
	    struct dir_attr *attr_list, const struct cp_options *x)
{
  struct dir_attr *p;
  char *dst_path;		/* A copy of CONST_DST_PATH we can change. */
  char *src_path;		/* The source name in `dst_path'. */
  uid_t myeuid = geteuid ();

  dst_path = (char *) alloca (strlen (const_dst_path) + 1);
  strcpy (dst_path, const_dst_path);
  src_path = dst_path + src_offset;

  for (p = attr_list; p; p = p->next)
    {
      struct stat src_sb;

      dst_path[p->slash_offset] = '\0';

      if ((*(x->xstat)) (src_path, &src_sb))
	{
	  error (0, errno, "%s", src_path);
	  return 1;
	}

      /* Adjust the times (and if possible, ownership) for the copy.
	 chown turns off set[ug]id bits for non-root,
	 so do the chmod last.  */

      if (x->preserve_timestamps)
	{
	  struct utimbuf utb;

	  /* There's currently no interface to set file timestamps with
	     better than 1-second resolution, so discard any fractional
	     part of the source timestamp.  */

	  utb.actime = src_sb.st_atime;
	  utb.modtime = src_sb.st_mtime;

	  if (utime (dst_path, &utb))
	    {
	      error (0, errno, _("preserving times for %s"), dst_path);
	      return 1;
	    }
	}

      if (x->preserve_owner_and_group)
	{
	  /* If non-root uses -p, it's ok if we can't preserve ownership.
	     But root probably wants to know, e.g. if NFS disallows it,
	     or if the target system doesn't support file ownership.  */
	  if (chown (dst_path, src_sb.st_uid, src_sb.st_gid)
	      && ((errno != EPERM && errno != EINVAL) || myeuid == 0))
	    {
	      error (0, errno, _("preserving ownership for %s"), dst_path);
	      return 1;
	    }
	}

      if (x->preserve_chmod_bits || p->is_new_dir)
	{
	  if (chmod (dst_path, src_sb.st_mode & x->umask_kill))
	    {
	      error (0, errno, _("preserving permissions for %s"), dst_path);
	      return 1;
	    }
	}

      dst_path[p->slash_offset] = '/';
    }
  return 0;
}

/* Ensure that the parent directory of CONST_DIRPATH exists, for
   the --parents option.

   SRC_OFFSET is the index in CONST_DIRPATH (which is a destination
   path) of the beginning of the source directory name.
   Create any leading directories that don't already exist,
   giving them permissions MODE.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory.
   The format should take two string arguments: the names of the
   source and destination directories.
   Creates a linked list of attributes of intermediate directories,
   *ATTR_LIST, for re_protect to use after calling copy.
   Sets *NEW_DST to 1 if this function creates parent of CONST_DIRPATH.

   Return 0 if parent of CONST_DIRPATH exists as a directory with the proper
   permissions when done, otherwise 1. */

static int
make_path_private (const char *const_dirpath, int src_offset, int mode,
		   const char *verbose_fmt_string, struct dir_attr **attr_list,
		   int *new_dst, int (*xstat)())
{
  struct stat stats;
  char *dirpath;		/* A copy of CONST_DIRPATH we can change. */
  char *src;			/* Source name in `dirpath'. */
  char *tmp_dst_dirname;	/* Leading path of `dirpath', malloc. */
  char *dst_dirname;		/* Leading path of `dirpath', alloca. */

  dirpath = (char *) alloca (strlen (const_dirpath) + 1);
  strcpy (dirpath, const_dirpath);

  src = dirpath + src_offset;

  tmp_dst_dirname = dir_name (dirpath);
  dst_dirname = (char *) alloca (strlen (tmp_dst_dirname) + 1);
  strcpy (dst_dirname, tmp_dst_dirname);
  free (tmp_dst_dirname);

  *attr_list = NULL;

  if ((*xstat) (dst_dirname, &stats))
    {
      /* Parent of CONST_DIRNAME does not exist.
	 Make all missing intermediate directories. */
      char *slash;

      slash = src;
      while (*slash == '/')
	slash++;
      while ((slash = strchr (slash, '/')))
	{
	  /* Add this directory to the list of directories whose modes need
	     fixing later. */
	  struct dir_attr *new =
	    (struct dir_attr *) xmalloc (sizeof (struct dir_attr));
	  new->slash_offset = slash - dirpath;
	  new->next = *attr_list;
	  *attr_list = new;

	  *slash = '\0';
	  if ((*xstat) (dirpath, &stats))
	    {
	      /* This element of the path does not exist.  We must set
		 *new_dst and new->is_new_dir inside this loop because,
		 for example, in the command `cp --parents ../a/../b/c e_dir',
		 make_path_private creates only e_dir/../a if ./b already
		 exists. */
	      *new_dst = 1;
	      new->is_new_dir = 1;
	      if (mkdir (dirpath, mode))
		{
		  error (0, errno, _("cannot make directory `%s'"), dirpath);
		  return 1;
		}
	      else
		{
		  if (verbose_fmt_string != NULL)
		    printf (verbose_fmt_string, src, dirpath);
		}
	    }
	  else if (!S_ISDIR (stats.st_mode))
	    {
	      error (0, 0, _("`%s' exists but is not a directory"), dirpath);
	      return 1;
	    }
	  else
	    {
	      new->is_new_dir = 0;
	      *new_dst = 0;
	    }
	  *slash++ = '/';

	  /* Avoid unnecessary calls to `stat' when given
	     pathnames containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}
    }

  /* We get here if the parent of `dirpath' already exists. */

  else if (!S_ISDIR (stats.st_mode))
    {
      error (0, 0, _("`%s' exists but is not a directory"), dst_dirname);
      return 1;
    }
  else
    {
      *new_dst = 0;
    }
  return 0;
}

/* Scan the arguments, and copy each by calling copy.
   Return 0 if successful, 1 if any errors occur. */

static int
do_copy (int argc, char **argv, const struct cp_options *x)
{
  char *dest;
  struct stat sb;
  int new_dst = 0;
  int ret = 0;

  if (optind >= argc)
    {
      error (0, 0, _("missing file arguments"));
      usage (1);
    }
  if (optind >= argc - 1)
    {
      error (0, 0, _("missing destination file"));
      usage (1);
    }

  dest = argv[argc - 1];

  if (lstat (dest, &sb))
    {
      if (errno != ENOENT)
	{
	  error (0, errno, "%s", dest);
	  return 1;
	}
      else
	new_dst = 1;
    }
  else
    {
      struct stat sbx;

      /* If `dest' is not a symlink to a nonexistent file, use
	 the results of stat instead of lstat, so we can copy files
	 into symlinks to directories. */
      if (stat (dest, &sbx) == 0)
	sb = sbx;
    }

  if (!new_dst && S_ISDIR (sb.st_mode))
    {
      /* cp file1...filen edir
	 Copy the files `file1' through `filen'
	 to the existing directory `edir'. */

      for (;;)
	{
	  char *arg;
	  char *ap;
	  char *dst_path;
	  int parent_exists = 1; /* True if dir_name (dst_path) exists. */
	  struct dir_attr *attr_list;
	  char *arg_in_concat = NULL;

	  arg = argv[optind];

	  strip_trailing_slashes (arg);

	  if (flag_path)
	    {
	      /* Append all of `arg' to `dest'.  */
	      dst_path = path_concat (dest, arg, &arg_in_concat);
	      if (dst_path == NULL)
		error (1, 0, _("virtual memory exhausted"));

	      /* For --parents, we have to make sure that the directory
	         dir_name (dst_path) exists.  We may have to create a few
	         leading directories. */
	      parent_exists = !make_path_private (dst_path,
						  arg_in_concat - dst_path,
						  0700,
						  (x->verbose
						   ? "%s -> %s\n" : NULL),
						  &attr_list, &new_dst,
						  x->xstat);
	    }
  	  else
  	    {
	      /* Append the last component of `arg' to `dest'.  */

	      ap = base_name (arg);
	      /* For `cp -R source/.. dest', don't copy into `dest/..'. */
	      dst_path = (STREQ (ap, "..")
			  ? xstrdup (dest)
			  : path_concat (dest, ap, NULL));
	    }

	  if (!parent_exists)
	    {
	      /* make_path_private failed, so don't even attempt the copy. */
	      ret = 1;
  	    }
	  else
	    {
	      int copy_into_self;
	      ret |= copy (arg, dst_path, new_dst, x, &copy_into_self, NULL);
	      forget_all ();

	      if (flag_path)
		{
		  ret |= re_protect (dst_path, arg_in_concat - dst_path,
				     attr_list, x);
		}
	    }

	  free (dst_path);
	  ++optind;
	  if (optind == argc - 1)
	    break;
	}
      return ret;
    }
  else if (argc - optind == 2)
    {
      char *new_dest;
      char *source;
      int unused;
      struct stat source_stats;

      if (flag_path)
	{
	  error (0, 0,
	       _("when preserving paths, last argument must be a directory"));
	  usage (1);
	}

      source = argv[optind];

      /* When the force and backup options have been specified and
	 the source and destination are the same name for an existing
	 regular file, convert the user's command, e.g.,
	 `cp --force --backup foo foo' to `cp --force foo fooSUFFIX'
	 where SUFFIX is determined by any version control options used.  */

      if (x->force
	  && x->backup_type != none
	  && STREQ (source, dest)
	  && !new_dst && S_ISREG (sb.st_mode))
	{
	  static struct cp_options x_tmp;

	  new_dest = find_backup_file_name (dest, x->backup_type);
	  /* Set x->backup_type to `none' so that the normal backup
	     mechanism is not used when performing the actual copy.
	     backup_type must be set to `none' only *after* the above
	     call to find_backup_file_name -- that function uses
	     backup_type to determine the suffix it applies.  */
	  x_tmp = *x;
	  x_tmp.backup_type = none;
	  x = &x_tmp;

	  if (new_dest == NULL)
	    error (1, 0, _("virtual memory exhausted"));
	}

      /* When the destination is specified with a trailing slash and the
	 source exists but is not a directory, convert the user's command
	 `cp source dest/' to `cp source dest/basename(source)'.  Doing
	 this ensures that the command `cp non-directory file/' will now
	 fail rather than performing the copy.  COPY diagnoses the case of
	 `cp directory non-directory'.  */

      else if (dest[strlen (dest) - 1] == '/'
	  && lstat (source, &source_stats) == 0
	  && !S_ISDIR (source_stats.st_mode))
	{
	  char *source_base;
	  char *tmp_source;

	  tmp_source = (char *) alloca (strlen (source) + 1);
	  strcpy (tmp_source, source);
	  strip_trailing_slashes (tmp_source);
	  source_base = base_name (tmp_source);

	  new_dest = (char *) alloca (strlen (dest)
				      + strlen (source_base) + 1);
	  stpcpy (stpcpy (new_dest, dest), source_base);
	}
      else
	{
	  new_dest = dest;
	}

      return copy (source, new_dest, new_dst, x, &unused, NULL);
    }
  else
    {
      error (0, 0,
	     _("copying multiple files, but last argument (%s) \
is not a directory"),
	     dest);
      usage (1);
    }

  /* unreachable */
  return 0;
}

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = 1;
  x->dereference = 1;
  x->force = 0;
  x->failed_unlink_is_fatal = 1;
  x->hard_link = 0;
  x->interactive = 0;
  x->myeuid = geteuid ();
  x->move_mode = 0;
  x->one_file_system = 0;

  x->preserve_owner_and_group = 0;
  x->preserve_chmod_bits = 0;
  x->preserve_timestamps = 0;

  x->require_preserve = 0;
  x->recursive = 0;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = 0;
  x->set_mode = 0;
  x->mode = 0;

  /* Find out the current file creation mask, to knock the right bits
     when using chmod.  The creation mask is set to be liberal, so
     that created directories can be written, even if it would not
     have been allowed with the mask this process was started with.  */
  x->umask_kill = 0777777 ^ umask (0);

  x->update = 0;
  x->verbose = 0;
}

int
main (int argc, char **argv)
{
  int c;
  int make_backups = 0;
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

  while ((c = getopt_long (argc, argv, "abdfilprsuvxPRS:V:", long_opts, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case CHAR_MAX + 1:
	  {
	    int i;

	    /* --sparse={never,auto,always}  */
	    i = argmatch (optarg, sparse_type_string);
	    if (i < 0)
	      {
		invalid_arg (_("sparse type"), optarg, i);
		usage (1);
	      }
	    x.sparse_mode = sparse_type[i];
	  }
	  break;

	case 'a':		/* Like -dpR. */
	  x.dereference = 0;
	  x.preserve_owner_and_group = 1;
	  x.preserve_chmod_bits = 1;
	  x.preserve_timestamps = 1;
	  x.require_preserve = 1;
	  x.recursive = 1;
	  x.copy_as_regular = 0;
	  break;

	case 'b':
	  make_backups = 1;
	  break;

	case 'd':
	  x.dereference = 0;
	  break;

	case 'f':
	  x.force = 1;
	  x.interactive = 0;
	  break;

	case 'i':
	  x.force = 0;
	  x.interactive = 1;
	  break;

	case 'l':
	  x.hard_link = 1;
	  break;

	case 'p':
	  x.preserve_owner_and_group = 1;
	  x.preserve_chmod_bits = 1;
	  x.preserve_timestamps = 1;
	  x.require_preserve = 1;
	  break;

	case 'P':
	  flag_path = 1;
	  break;

	case 'r':
	  x.recursive = 1;
	  x.copy_as_regular = 1;
	  break;

	case 'R':
	  x.recursive = 1;
	  x.copy_as_regular = 0;
	  break;

	case 's':
#ifdef S_ISLNK
	  x.symbolic_link = 1;
#else
	  error (1, 0, _("symbolic links are not supported on this system"));
#endif
	  break;

	case 'u':
	  x.update = 1;
	  break;

	case 'v':
	  x.verbose = 1;
	  break;

	case 'x':
	  x.one_file_system = 1;
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
      printf ("cp (%s) %s\n", GNU_PACKAGE, VERSION);
      close_stdout ();
      exit (0);
    }

  if (show_help)
    usage (0);

  if (x.hard_link && x.symbolic_link)
    {
      error (0, 0, _("cannot make both hard and symbolic links"));
      usage (1);
    }

  x.backup_type = (make_backups ? get_version (version) : none);

  if (x.preserve_chmod_bits == 1)
    x.umask_kill = 0777777;

  /* The key difference between -d (--no-dereference) and not is the version
     of `stat' to call.  */

  if (x.dereference)
    x.xstat = stat;
  else
    x.xstat = lstat;

  /* Allocate space for remembering copied and created files.  */

  hash_init (INITIAL_HASH_MODULE, INITIAL_ENTRY_TAB_SIZE);

  exit_status |= do_copy (argc, argv, &x);

  if (x.verbose)
    close_stdout ();
  exit (exit_status);
}
