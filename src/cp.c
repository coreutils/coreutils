/* cp.c  -- file copying (main routines)
   Copyright (C) 89, 90, 91, 1995-2003 Free Software Foundation.

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
#include "argmatch.h"
#include "backupfile.h"
#include "copy.h"
#include "cp-hash.h"
#include "error.h"
#include "dirname.h"
#include "path-concat.h"
#include "quote.h"

#define ASSIGN_BASENAME_STRDUPA(Dest, File_name)	\
  do							\
    {							\
      char *tmp_abns_;					\
      ASSIGN_STRDUPA (tmp_abns_, (File_name));		\
      strip_trailing_slashes (tmp_abns_);		\
      Dest = base_name (tmp_abns_);			\
    }							\
  while (0)

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "cp"

#define AUTHORS N_ ("Torbjorn Granlund, David MacKenzie, and Jim Meyering")

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

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  COPY_CONTENTS_OPTION = CHAR_MAX + 1,
  NO_PRESERVE_ATTRIBUTES_OPTION,
  PARENTS_OPTION,
  PRESERVE_ATTRIBUTES_OPTION,
  REPLY_OPTION,
  SPARSE_OPTION,
  STRIP_TRAILING_SLASHES_OPTION,
  TARGET_DIRECTORY_OPTION,
  UNLINK_DEST_BEFORE_OPENING
};

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

/* The invocation name of this program.  */
char *program_name;

/* If nonzero, the command "cp x/e_file e_dir" uses "e_dir/x/e_file"
   as its destination instead of the usual "e_dir/e_file." */
static int flag_path = 0;

/* Remove any trailing slashes from each SOURCE argument.  */
static int remove_trailing_slashes;

static char const *const sparse_type_string[] =
{
  "never", "auto", "always", 0
};

static enum Sparse_type const sparse_type[] =
{
  SPARSE_NEVER, SPARSE_AUTO, SPARSE_ALWAYS
};

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

/* The error code to return to the system. */
static int exit_status = 0;

static struct option const long_opts[] =
{
  {"archive", no_argument, NULL, 'a'},
  {"backup", optional_argument, NULL, 'b'},
  {"copy-contents", no_argument, NULL, COPY_CONTENTS_OPTION},
  {"dereference", no_argument, NULL, 'L'},
  {"force", no_argument, NULL, 'f'},
  {"interactive", no_argument, NULL, 'i'},
  {"link", no_argument, NULL, 'l'},
  {"no-dereference", no_argument, NULL, 'P'},
  {"no-preserve", required_argument, NULL, NO_PRESERVE_ATTRIBUTES_OPTION},
  {"one-file-system", no_argument, NULL, 'x'},
  {"parents", no_argument, NULL, PARENTS_OPTION},
  {"path", no_argument, NULL, PARENTS_OPTION},   /* Deprecated.  */
  {"preserve", optional_argument, NULL, PRESERVE_ATTRIBUTES_OPTION},
  {"recursive", no_argument, NULL, 'R'},
  {"remove-destination", no_argument, NULL, UNLINK_DEST_BEFORE_OPENING},
  {"reply", required_argument, NULL, REPLY_OPTION},
  {"sparse", required_argument, NULL, SPARSE_OPTION},
  {"strip-trailing-slashes", no_argument, NULL, STRIP_TRAILING_SLASHES_OPTION},
  {"suffix", required_argument, NULL, 'S'},
  {"symbolic-link", no_argument, NULL, 's'},
  {"target-directory", required_argument, NULL, TARGET_DIRECTORY_OPTION},
  {"update", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {"version-control", required_argument, NULL, 'V'}, /* Deprecated. FIXME. */
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

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
Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -a, --archive                same as -dpR\n\
      --backup[=CONTROL]       make a backup of each existing destination file\n\
  -b                           like --backup but does not accept an argument\n\
      --copy-contents          copy contents of special files when recursive\n\
  -d                           same as --no-dereference --preserve=link\n\
"), stdout);
      fputs (_("\
      --no-dereference         never follow symbolic links\n\
  -f, --force                  if an existing destination file cannot be\n\
                                 opened, remove it and try again\n\
  -i, --interactive            prompt before overwrite\n\
  -H                           follow command-line symbolic links\n\
"), stdout);
      fputs (_("\
  -l, --link                   link files instead of copying\n\
  -L, --dereference            always follow symbolic links\n\
  -p                           same as --preserve=mode,ownership,timestamps\n\
      --preserve[=ATTR_LIST]   preserve the specified attributes (default:\n\
                                 mode,ownership,timestamps), if possible\n\
                                 additional attributes: links, all\n\
"), stdout);
      fputs (_("\
      --no-preserve=ATTR_LIST  don't preserve the specified attributes\n\
      --parents                append source path to DIRECTORY\n\
  -P                           same as `--no-dereference'\n\
"), stdout);
      fputs (_("\
  -R, -r, --recursive          copy directories recursively\n\
      --remove-destination     remove each existing destination file before\n\
                                 attempting to open it (contrast with --force)\n\
"), stdout);
      fputs (_("\
      --reply={yes,no,query}   specify how to handle the prompt about an\n\
                                 existing destination file\n\
      --sparse=WHEN            control creation of sparse files\n\
      --strip-trailing-slashes remove any trailing slashes from each SOURCE\n\
                                 argument\n\
"), stdout);
      fputs (_("\
  -s, --symbolic-link          make symbolic links instead of copying\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
      --target-directory=DIRECTORY  move all SOURCE arguments into DIRECTORY\n\
"), stdout);
      fputs (_("\
  -u, --update                 copy only when the SOURCE file is newer\n\
                                 than the destination file or when the\n\
                                 destination file is missing\n\
  -v, --verbose                explain what is being done\n\
  -x, --one-file-system        stay on this file system\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
By default, sparse SOURCE files are detected by a crude heuristic and the\n\
corresponding DEST file is made sparse as well.  That is the behavior\n\
selected by --sparse=auto.  Specify --sparse=always to create a sparse DEST\n\
file whenever the SOURCE file contains a long enough sequence of zero bytes.\n\
Use --sparse=never to inhibit creation of sparse files.\n\
\n\
"), stdout);
      fputs (_("\
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
      fputs (_("\
\n\
As a special case, cp makes a backup of SOURCE when the force and backup\n\
options are given and SOURCE and DEST are the same name for an existing,\n\
regular file.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
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
	  error (0, errno, _("failed to get attributes of %s"),
		 quote (src_path));
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
	      error (0, errno, _("failed to preserve times for %s"),
		     quote (dst_path));
	      return 1;
	    }
	}

      if (x->preserve_ownership)
	{
	  /* If non-root uses -p, it's ok if we can't preserve ownership.
	     But root probably wants to know, e.g. if NFS disallows it,
	     or if the target system doesn't support file ownership.  */
	  if (chown (dst_path, src_sb.st_uid, src_sb.st_gid)
	      && ((errno != EPERM && errno != EINVAL) || myeuid == 0))
	    {
	      error (0, errno, _("failed to preserve ownership for %s"),
		     quote (dst_path));
	      return 1;
	    }
	}

      if (x->preserve_mode || p->is_new_dir)
	{
	  if (chmod (dst_path, src_sb.st_mode & x->umask_kill))
	    {
	      error (0, errno, _("failed to preserve permissions for %s"),
		     quote (dst_path));
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

/* FIXME: find a way to synch this function with the one in lib/makepath.c. */

static int
make_path_private (const char *const_dirpath, int src_offset, int mode,
		   const char *verbose_fmt_string, struct dir_attr **attr_list,
		   int *new_dst, int (*xstat)())
{
  struct stat stats;
  char *dirpath;		/* A copy of CONST_DIRPATH we can change. */
  char *src;			/* Source name in `dirpath'. */
  char *dst_dirname;		/* Leading path of `dirpath'. */
  size_t dirlen;		/* Length of leading path of `dirpath'. */

  dirpath = (char *) alloca (strlen (const_dirpath) + 1);
  strcpy (dirpath, const_dirpath);

  src = dirpath + src_offset;

  dirlen = dir_len (dirpath);
  dst_dirname = (char *) alloca (dirlen + 1);
  memcpy (dst_dirname, dirpath, dirlen);
  dst_dirname[dirlen] = '\0';

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
		  error (0, errno, _("cannot make directory %s"),
			 quote (dirpath));
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
	      error (0, 0, _("%s exists but is not a directory"),
		     quote (dirpath));
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
      error (0, 0, _("%s exists but is not a directory"), quote (dst_dirname));
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
do_copy (int n_files, char **file, const char *target_directory,
	 struct cp_options *x)
{
  const char *dest;
  struct stat sb;
  int new_dst = 0;
  int ret = 0;
  int dest_is_dir = 0;

  if (n_files <= 0)
    {
      error (0, 0, _("missing file argument"));
      usage (EXIT_FAILURE);
    }
  if (n_files == 1 && !target_directory)
    {
      error (0, 0, _("missing destination file"));
      usage (EXIT_FAILURE);
    }

  if (target_directory)
    dest = target_directory;
  else
    {
      dest = file[n_files - 1];
      --n_files;
    }

  /* Initialize these hash tables only if we'll need them.
     The problems they're used to detect can arise only if
     there are two or more files to copy.  */
  if (n_files >= 2)
    {
      dest_info_init (x);
      src_info_init (x);
    }

  if (lstat (dest, &sb))
    {
      if (errno != ENOENT)
	{
	  error (0, errno, _("accessing %s"), quote (dest));
	  return 1;
	}

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

      dest_is_dir = S_ISDIR (sb.st_mode);
    }

  if (!dest_is_dir)
    {
      if (target_directory)
	{
	  error (0, 0, _("%s: specified target is not a directory"),
		 quote (dest));
	  usage (EXIT_FAILURE);
	}

      if (n_files > 1)
	{
	  error (0, 0,
	 _("copying multiple files, but last argument %s is not a directory"),
	     quote (dest));
	  usage (EXIT_FAILURE);
	}
    }

  if (dest_is_dir)
    {
      /* cp file1...filen edir
	 Copy the files `file1' through `filen'
	 to the existing directory `edir'. */
      int i;

      for (i = 0; i < n_files; i++)
	{
	  char *dst_path;
	  int parent_exists = 1; /* True if dir_name (dst_path) exists. */
	  struct dir_attr *attr_list;
	  char *arg_in_concat = NULL;
	  char *arg = file[i];

	  /* Trailing slashes are meaningful (i.e., maybe worth preserving)
	     only in the source file names.  */
	  if (remove_trailing_slashes)
	    strip_trailing_slashes (arg);

	  if (flag_path)
	    {
	      char *arg_no_trailing_slash;

	      /* Use `arg' without trailing slashes in constructing destination
		 file names.  Otherwise, we can end up trying to create a
		 directory via `mkdir ("dst/foo/"...', which is not portable.
		 It fails, due to the trailing slash, on at least
		 NetBSD 1.[34] systems.  */
	      ASSIGN_STRDUPA (arg_no_trailing_slash, arg);
	      strip_trailing_slashes (arg_no_trailing_slash);

	      /* Append all of `arg' (minus any trailing slash) to `dest'.  */
	      dst_path = path_concat (dest, arg_no_trailing_slash,
				      &arg_in_concat);
	      if (dst_path == NULL)
		xalloc_die ();

	      /* For --parents, we have to make sure that the directory
	         dir_name (dst_path) exists.  We may have to create a few
	         leading directories. */
	      parent_exists = !make_path_private (dst_path,
						  arg_in_concat - dst_path,
						  S_IRWXU,
						  (x->verbose
						   ? "%s -> %s\n" : NULL),
						  &attr_list, &new_dst,
						  x->xstat);
	    }
  	  else
  	    {
	      char *arg_base;
	      /* Append the last component of `arg' to `dest'.  */

	      ASSIGN_BASENAME_STRDUPA (arg_base, arg);
	      /* For `cp -R source/.. dest', don't copy into `dest/..'. */
	      dst_path = (STREQ (arg_base, "..")
			  ? xstrdup (dest)
			  : path_concat (dest, arg_base, NULL));
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

	      if (flag_path)
		{
		  ret |= re_protect (dst_path, arg_in_concat - dst_path,
				     attr_list, x);
		}
	    }

	  free (dst_path);
	}
      return ret;
    }
  else /* if (n_files == 1) */
    {
      char *new_dest;
      char *source;
      int unused;
      struct stat source_stats;

      if (flag_path)
	{
	  error (0, 0,
	       _("when preserving paths, the destination must be a directory"));
	  usage (EXIT_FAILURE);
	}

      source = file[0];

      /* When the force and backup options have been specified and
	 the source and destination are the same name for an existing
	 regular file, convert the user's command, e.g.,
	 `cp --force --backup foo foo' to `cp --force foo fooSUFFIX'
	 where SUFFIX is determined by any version control options used.  */

      if (x->unlink_dest_after_failed_open
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
	    xalloc_die ();
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

	  ASSIGN_BASENAME_STRDUPA (source_base, source);
	  new_dest = (char *) alloca (strlen (dest)
				      + strlen (source_base) + 1);
	  stpcpy (stpcpy (new_dest, dest), source_base);
	}
      else
	{
	  new_dest = (char *) dest;
	}

      return copy (source, new_dest, new_dst, x, &unused, NULL);
    }

  /* unreachable */
}

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = 1;
  x->dereference = DEREF_UNDEFINED;
  x->unlink_dest_before_opening = 0;
  x->unlink_dest_after_failed_open = 0;
  x->hard_link = 0;
  x->interactive = I_UNSPECIFIED;
  x->myeuid = geteuid ();
  x->move_mode = 0;
  x->one_file_system = 0;

  x->preserve_ownership = 0;
  x->preserve_links = 0;
  x->preserve_mode = 0;
  x->preserve_timestamps = 0;

  x->require_preserve = 0;
  x->recursive = 0;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = 0;
  x->set_mode = 0;
  x->mode = 0;

  /* Not used.  */
  x->stdin_tty = 0;

  /* Find out the current file creation mask, to knock the right bits
     when using chmod.  The creation mask is set to be liberal, so
     that created directories can be written, even if it would not
     have been allowed with the mask this process was started with.  */
  x->umask_kill = ~ umask (0);

  x->update = 0;
  x->verbose = 0;
  x->dest_info = NULL;
  x->src_info = NULL;
}

/* Given a string, ARG, containing a comma-separated list of arguments
   to the --preserve option, set the appropriate fields of X to ON_OFF.  */
static void
decode_preserve_arg (char const *arg, struct cp_options *x, int on_off)
{
  enum File_attribute
    {
      PRESERVE_MODE,
      PRESERVE_TIMESTAMPS,
      PRESERVE_OWNERSHIP,
      PRESERVE_LINK,
      PRESERVE_ALL
    };
  static enum File_attribute const preserve_vals[] =
    {
      PRESERVE_MODE, PRESERVE_TIMESTAMPS,
      PRESERVE_OWNERSHIP, PRESERVE_LINK, PRESERVE_ALL
    };

  /* Valid arguments to the `--preserve' option. */
  static char const* const preserve_args[] =
    {
      "mode", "timestamps",
      "ownership", "links", "all", 0
    };

  char *arg_writable = xstrdup (arg);
  char *s = arg_writable;
  do
    {
      /* find next comma */
      char *comma = strchr (s, ',');
      enum File_attribute val;

      /* If we found a comma, put a NUL in its place and advance.  */
      if (comma)
	*comma++ = 0;

      /* process S.  */
      val = XARGMATCH ("--preserve", s, preserve_args, preserve_vals);
      switch (val)
	{
	case PRESERVE_MODE:
	  x->preserve_mode = on_off;
	  break;

	case PRESERVE_TIMESTAMPS:
	  x->preserve_timestamps = on_off;
	  break;

	case PRESERVE_OWNERSHIP:
	  x->preserve_ownership = on_off;
	  break;

	case PRESERVE_LINK:
	  x->preserve_links = on_off;
	  break;

	case PRESERVE_ALL:
	  x->preserve_mode = on_off;
	  x->preserve_timestamps = on_off;
	  x->preserve_ownership = on_off;
	  x->preserve_links = on_off;
	  break;

	default:
	  abort ();
	}
      s = comma;
    }
  while (s);

  free (arg_writable);
}

int
main (int argc, char **argv)
{
  int c;
  int make_backups = 0;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  struct cp_options x;
  int copy_contents = 0;
  char *target_directory = NULL;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  cp_option_init (&x);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  while ((c = getopt_long (argc, argv, "abdfHilLprsuvxPRS:V:", long_opts, NULL))
	 != -1)
    {
      switch (c)
	{
	case 0:
	  break;

	case SPARSE_OPTION:
	  x.sparse_mode = XARGMATCH ("--sparse", optarg,
				     sparse_type_string, sparse_type);
	  break;

	case 'a':		/* Like -dpPR. */
	  x.dereference = DEREF_NEVER;
	  x.preserve_links = 1;
	  x.preserve_ownership = 1;
	  x.preserve_mode = 1;
	  x.preserve_timestamps = 1;
	  x.require_preserve = 1;
	  x.recursive = 1;
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

	case COPY_CONTENTS_OPTION:
	  copy_contents = 1;
	  break;

	case 'd':
	  x.preserve_links = 1;
	  x.dereference = DEREF_NEVER;
	  break;

	case 'f':
	  x.unlink_dest_after_failed_open = 1;
	  break;

	case 'H':
	  x.dereference = DEREF_COMMAND_LINE_ARGUMENTS;
	  break;

	case 'i':
	  x.interactive = I_ASK_USER;
	  break;

	case 'l':
	  x.hard_link = 1;
	  break;

	case 'L':
	  x.dereference = DEREF_ALWAYS;
	  break;

	case 'P':
	  x.dereference = DEREF_NEVER;
	  break;

	case NO_PRESERVE_ATTRIBUTES_OPTION:
	  decode_preserve_arg (optarg, &x, 0);
	  break;

	case PRESERVE_ATTRIBUTES_OPTION:
	  if (optarg == NULL)
	    {
	      /* Fall through to the case for `p' below.  */
	    }
	  else
	    {
	      decode_preserve_arg (optarg, &x, 1);
	      x.require_preserve = 1;
	      break;
	    }

	case 'p':
	  x.preserve_ownership = 1;
	  x.preserve_mode = 1;
	  x.preserve_timestamps = 1;
	  x.require_preserve = 1;
	  break;

	case PARENTS_OPTION:
	  flag_path = 1;
	  break;

	case 'r':
	case 'R':
	  x.recursive = 1;
	  break;

	case REPLY_OPTION:
	  x.interactive = XARGMATCH ("--reply", optarg,
				     reply_args, reply_vals);
	  break;

	case UNLINK_DEST_BEFORE_OPENING:
	  x.unlink_dest_before_opening = 1;
	  break;

	case STRIP_TRAILING_SLASHES_OPTION:
	  remove_trailing_slashes = 1;
	  break;

	case 's':
#ifdef S_ISLNK
	  x.symbolic_link = 1;
#else
	  error (EXIT_FAILURE, 0,
		 _("symbolic links are not supported on this system"));
#endif
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

	case 'x':
	  x.one_file_system = 1;
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

  if (x.hard_link && x.symbolic_link)
    {
      error (0, 0, _("cannot make both hard and symbolic links"));
      usage (EXIT_FAILURE);
    }

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  x.backup_type = (make_backups
		   ? xget_version (_("backup type"),
				   version_control_string)
		   : none);

  if (x.preserve_mode == 1)
    x.umask_kill = ~ (mode_t) 0;

  if (x.dereference == DEREF_UNDEFINED)
    {
      if (x.recursive)
	/* This is compatible with FreeBSD.  */
	x.dereference = DEREF_NEVER;
      else
	x.dereference = DEREF_ALWAYS;
    }

  /* The key difference between -d (--no-dereference) and not is the version
     of `stat' to call.  */

  if (x.dereference == DEREF_NEVER)
    x.xstat = lstat;
  else
    {
      /* For DEREF_COMMAND_LINE_ARGUMENTS, x.xstat must be stat for
	 each command line argument, but must later be `lstat' for
	 any symlinks that are found via recursive traversal.  */
      x.xstat = stat;
    }

  if (x.recursive)
    x.copy_as_regular = copy_contents;

  /* If --force (-f) was specified and we're in link-creation mode,
     first remove any existing destination file.  */
  if (x.unlink_dest_after_failed_open && (x.hard_link || x.symbolic_link))
    x.unlink_dest_before_opening = 1;

  /* Allocate space for remembering copied and created files.  */

  hash_init ();

  exit_status |= do_copy (argc - optind, argv + optind, target_directory, &x);

  forget_all ();

  exit (exit_status);
}
