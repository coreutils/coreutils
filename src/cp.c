/* cp.c  -- file copying (main routines)
   Copyright (C) 89, 90, 91, 1995-2005 Free Software Foundation.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Torbjorn Granlund, David MacKenzie, and Jim Meyering. */

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
#include "filenamecat.h"
#include "quote.h"
#include "quotearg.h"
#include "stat-time.h"
#include "utimens.h"

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

#define AUTHORS "Torbjorn Granlund", "David MacKenzie", "Jim Meyering"

/* Used by do_copy, make_dir_parents_private, and re_protect
   to keep a list of leading directories whose protections
   need to be fixed after copying. */
struct dir_attr
{
  bool is_new_dir;
  size_t slash_offset;
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
  UNLINK_DEST_BEFORE_OPENING
};

/* Initial number of entries in each hash table entry's table of inodes.  */
#define INITIAL_HASH_MODULE 100

/* Initial number of entries in the inode hash table.  */
#define INITIAL_ENTRY_TAB_SIZE 70

/* The invocation name of this program.  */
char *program_name;

/* If true, the command "cp x/e_file e_dir" uses "e_dir/x/e_file"
   as its destination instead of the usual "e_dir/e_file." */
static bool parents_option = false;

/* Remove any trailing slashes from each SOURCE argument.  */
static bool remove_trailing_slashes;

static char const *const sparse_type_string[] =
{
  "never", "auto", "always", NULL
};
static enum Sparse_type const sparse_type[] =
{
  SPARSE_NEVER, SPARSE_AUTO, SPARSE_ALWAYS
};
ARGMATCH_VERIFY (sparse_type_string, sparse_type);

/* Valid arguments to the `--reply' option. */
static char const* const reply_args[] =
{
  "yes", "no", "query", NULL
};
/* The values that correspond to the above strings. */
static int const reply_vals[] =
{
  I_ALWAYS_YES, I_ALWAYS_NO, I_ASK_USER
};
ARGMATCH_VERIFY (reply_args, reply_vals);

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
  {"no-target-directory", no_argument, NULL, 'T'},
  {"one-file-system", no_argument, NULL, 'x'},
  {"parents", no_argument, NULL, PARENTS_OPTION},
  {"path", no_argument, NULL, PARENTS_OPTION},   /* Deprecated.  */
  {"preserve", optional_argument, NULL, PRESERVE_ATTRIBUTES_OPTION},
  {"recursive", no_argument, NULL, 'R'},
  {"remove-destination", no_argument, NULL, UNLINK_DEST_BEFORE_OPENING},
  {"reply", required_argument, NULL, REPLY_OPTION}, /* Deprecated 2005-07-03,
						       remove in 2008. */
  {"sparse", required_argument, NULL, SPARSE_OPTION},
  {"strip-trailing-slashes", no_argument, NULL, STRIP_TRAILING_SLASHES_OPTION},
  {"suffix", required_argument, NULL, 'S'},
  {"symbolic-link", no_argument, NULL, 's'},
  {"target-directory", required_argument, NULL, 't'},
  {"update", no_argument, NULL, 'u'},
  {"verbose", no_argument, NULL, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

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
  -f, --force                  if an existing destination file cannot be\n\
                                 opened, remove it and try again\n\
  -i, --interactive            prompt before overwrite\n\
  -H                           follow command-line symbolic links\n\
"), stdout);
      fputs (_("\
  -l, --link                   link files instead of copying\n\
  -L, --dereference            always follow symbolic links\n\
"), stdout);
      fputs (_("\
  -P, --no-dereference         never follow symbolic links\n\
"), stdout);
      fputs (_("\
  -p                           same as --preserve=mode,ownership,timestamps\n\
      --preserve[=ATTR_LIST]   preserve the specified attributes (default:\n\
                                 mode,ownership,timestamps), if possible\n\
                                 additional attributes: links, all\n\
"), stdout);
      fputs (_("\
      --no-preserve=ATTR_LIST  don't preserve the specified attributes\n\
      --parents                use full source file name under DIRECTORY\n\
"), stdout);
      fputs (_("\
  -R, -r, --recursive          copy directories recursively\n\
      --remove-destination     remove each existing destination file before\n\
                                 attempting to open it (contrast with --force)\n\
"), stdout);
      fputs (_("\
      --sparse=WHEN            control creation of sparse files\n\
      --strip-trailing-slashes remove any trailing slashes from each SOURCE\n\
                                 argument\n\
"), stdout);
      fputs (_("\
  -s, --symbolic-link          make symbolic links instead of copying\n\
  -S, --suffix=SUFFIX          override the usual backup suffix\n\
  -t, --target-directory=DIRECTORY  copy all SOURCE arguments into DIRECTORY\n\
  -T, --no-target-directory    treat DEST as a normal file\n\
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

/* Ensure that the parent directories of CONST_DST_NAME have the
   correct protections, for the --parents option.  This is done
   after all copying has been completed, to allow permissions
   that don't include user write/execute.

   SRC_OFFSET is the index in CONST_DST_NAME of the beginning of the
   source directory name.

   ATTR_LIST is a null-terminated linked list of structures that
   indicates the end of the filename of each intermediate directory
   in CONST_DST_NAME that may need to have its attributes changed.
   The command `cp --parents --preserve a/b/c d/e_dir' changes the
   attributes of the directories d/e_dir/a and d/e_dir/a/b to match
   the corresponding source directories regardless of whether they
   existed before the `cp' command was given.

   Return true if the parent of CONST_DST_NAME and any intermediate
   directories specified by ATTR_LIST have the proper permissions
   when done.  */

static bool
re_protect (char const *const_dst_name, size_t src_offset,
	    struct dir_attr *attr_list, const struct cp_options *x)
{
  struct dir_attr *p;
  char *dst_name;		/* A copy of CONST_DST_NAME we can change. */
  char *src_name;		/* The source name in `dst_name'. */

  ASSIGN_STRDUPA (dst_name, const_dst_name);
  src_name = dst_name + src_offset;

  for (p = attr_list; p; p = p->next)
    {
      struct stat src_sb;

      dst_name[p->slash_offset] = '\0';

      if (XSTAT (x, src_name, &src_sb))
	{
	  error (0, errno, _("failed to get attributes of %s"),
		 quote (src_name));
	  return false;
	}

      /* Adjust the times (and if possible, ownership) for the copy.
	 chown turns off set[ug]id bits for non-root,
	 so do the chmod last.  */

      if (x->preserve_timestamps)
	{
	  struct timespec timespec[2];

	  timespec[0] = get_stat_atime (&src_sb);
	  timespec[1] = get_stat_mtime (&src_sb);

	  if (utimens (dst_name, timespec))
	    {
	      error (0, errno, _("failed to preserve times for %s"),
		     quote (dst_name));
	      return false;
	    }
	}

      if (x->preserve_ownership)
	{
	  if (chown (dst_name, src_sb.st_uid, src_sb.st_gid) != 0
	      && ! chown_failure_ok (x))
	    {
	      error (0, errno, _("failed to preserve ownership for %s"),
		     quote (dst_name));
	      return false;
	    }
	}

      if (x->preserve_mode | p->is_new_dir)
	{
	  if (chmod (dst_name, src_sb.st_mode & x->umask_kill))
	    {
	      error (0, errno, _("failed to preserve permissions for %s"),
		     quote (dst_name));
	      return false;
	    }
	}

      dst_name[p->slash_offset] = '/';
    }
  return true;
}

/* Ensure that the parent directory of CONST_DIR exists, for
   the --parents option.

   SRC_OFFSET is the index in CONST_DIR (which is a destination
   directory) of the beginning of the source directory name.
   Create any leading directories that don't already exist,
   giving them permissions MODE.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory.
   The format should take two string arguments: the names of the
   source and destination directories.
   Creates a linked list of attributes of intermediate directories,
   *ATTR_LIST, for re_protect to use after calling copy.
   Sets *NEW_DST if this function creates parent of CONST_DIR.

   Return true if parent of CONST_DIR exists as a directory with the proper
   permissions when done.  */

/* FIXME: Synch this function with the one in ../lib/mkdir-p.c.  */

static bool
make_dir_parents_private (char const *const_dir, size_t src_offset,
			  mode_t mode, char const *verbose_fmt_string,
			  struct dir_attr **attr_list, bool *new_dst,
			  int (*xstat) ())
{
  struct stat stats;
  char *dir;		/* A copy of CONST_DIR we can change.  */
  char *src;		/* Source name in DIR.  */
  char *dst_dir;	/* Leading directory of DIR.  */
  size_t dirlen;	/* Length of DIR.  */

  ASSIGN_STRDUPA (dir, const_dir);

  src = dir + src_offset;

  dirlen = dir_len (dir);
  dst_dir = alloca (dirlen + 1);
  memcpy (dst_dir, dir, dirlen);
  dst_dir[dirlen] = '\0';

  *attr_list = NULL;

  if ((*xstat) (dst_dir, &stats))
    {
      /* A parent of CONST_DIR does not exist.
	 Make all missing intermediate directories. */
      char *slash;

      slash = src;
      while (*slash == '/')
	slash++;
      while ((slash = strchr (slash, '/')))
	{
	  /* Add this directory to the list of directories whose modes need
	     fixing later. */
	  struct dir_attr *new = xmalloc (sizeof *new);
	  new->slash_offset = slash - dir;
	  new->next = *attr_list;
	  *attr_list = new;

	  *slash = '\0';
	  if ((*xstat) (dir, &stats))
	    {
	      /* This component does not exist.  We must set
		 *new_dst and new->is_new_dir inside this loop because,
		 for example, in the command `cp --parents ../a/../b/c e_dir',
		 make_dir_parents_private creates only e_dir/../a if
		 ./b already exists. */
	      *new_dst = true;
	      new->is_new_dir = true;
	      if (mkdir (dir, mode))
		{
		  error (0, errno, _("cannot make directory %s"),
			 quote (dir));
		  return false;
		}
	      else
		{
		  if (verbose_fmt_string != NULL)
		    printf (verbose_fmt_string, src, dir);
		}
	    }
	  else if (!S_ISDIR (stats.st_mode))
	    {
	      error (0, 0, _("%s exists but is not a directory"),
		     quote (dir));
	      return false;
	    }
	  else
	    {
	      new->is_new_dir = false;
	      *new_dst = false;
	    }
	  *slash++ = '/';

	  /* Avoid unnecessary calls to `stat' when given
	     file names containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}
    }

  /* We get here if the parent of DIR already exists.  */

  else if (!S_ISDIR (stats.st_mode))
    {
      error (0, 0, _("%s exists but is not a directory"), quote (dst_dir));
      return false;
    }
  else
    {
      *new_dst = false;
    }
  return true;
}

/* FILE is the last operand of this command.
   Return true if FILE is a directory.
   But report an error and exit if there is a problem accessing FILE,
   or if FILE does not exist but would have to refer to an existing
   directory if it referred to anything at all.

   If the file exists, store the file's status into *ST.
   Otherwise, set *NEW_DST.  */

static bool
target_directory_operand (char const *file, struct stat *st, bool *new_dst)
{
  char const *b = base_name (file);
  size_t blen = strlen (b);
  bool looks_like_a_dir = (blen == 0 || ISSLASH (b[blen - 1]));
  int err = (stat (file, st) == 0 ? 0 : errno);
  bool is_a_dir = !err && S_ISDIR (st->st_mode);
  if (err)
    {
      if (err != ENOENT)
	error (EXIT_FAILURE, err, _("accessing %s"), quote (file));
      *new_dst = true;
    }
  if (is_a_dir < looks_like_a_dir)
    error (EXIT_FAILURE, err, _("target %s is not a directory"), quote (file));
  return is_a_dir;
}

/* Scan the arguments, and copy each by calling copy.
   Return true if successful.  */

static bool
do_copy (int n_files, char **file, const char *target_directory,
	 bool no_target_directory, struct cp_options *x)
{
  struct stat sb;
  bool new_dst = false;
  bool ok = true;

  if (n_files <= !target_directory)
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
  else if (!target_directory)
    {
      if (2 <= n_files
	  && target_directory_operand (file[n_files - 1], &sb, &new_dst))
	target_directory = file[--n_files];
      else if (2 < n_files)
	error (EXIT_FAILURE, 0, _("target %s is not a directory"),
	       quote (file[n_files - 1]));
    }

  if (target_directory)
    {
      /* cp file1...filen edir
	 Copy the files `file1' through `filen'
	 to the existing directory `edir'. */
      int i;
      int (*xstat)() = (x->dereference == DEREF_COMMAND_LINE_ARGUMENTS
			|| x->dereference == DEREF_ALWAYS
			? stat
			: lstat);

      /* Initialize these hash tables only if we'll need them.
	 The problems they're used to detect can arise only if
	 there are two or more files to copy.  */
      if (2 <= n_files)
	{
	  dest_info_init (x);
	  src_info_init (x);
	}

      for (i = 0; i < n_files; i++)
	{
	  char *dst_name;
	  bool parent_exists = true;  /* True if dir_name (dst_name) exists. */
	  struct dir_attr *attr_list;
	  char *arg_in_concat = NULL;
	  char *arg = file[i];

	  /* Trailing slashes are meaningful (i.e., maybe worth preserving)
	     only in the source file names.  */
	  if (remove_trailing_slashes)
	    strip_trailing_slashes (arg);

	  if (parents_option)
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
	      dst_name = file_name_concat (target_directory,
					   arg_no_trailing_slash,
					   &arg_in_concat);

	      /* For --parents, we have to make sure that the directory
	         dir_name (dst_name) exists.  We may have to create a few
	         leading directories. */
	      parent_exists =
		(make_dir_parents_private
		 (dst_name, arg_in_concat - dst_name, S_IRWXU,
		  (x->verbose ? "%s -> %s\n" : NULL),
		  &attr_list, &new_dst, xstat));
	    }
	  else
	    {
	      char *arg_base;
	      /* Append the last component of `arg' to `target_directory'.  */

	      ASSIGN_BASENAME_STRDUPA (arg_base, arg);
	      /* For `cp -R source/.. dest', don't copy into `dest/..'. */
	      dst_name = (STREQ (arg_base, "..")
			  ? xstrdup (target_directory)
			  : file_name_concat (target_directory, arg_base,
					      NULL));
	    }

	  if (!parent_exists)
	    {
	      /* make_dir_parents_private failed, so don't even
		 attempt the copy.  */
	      ok = false;
	    }
	  else
	    {
	      bool copy_into_self;
	      ok &= copy (arg, dst_name, new_dst, x, &copy_into_self, NULL);

	      if (parents_option)
		ok &= re_protect (dst_name, arg_in_concat - dst_name,
				  attr_list, x);
	    }

	  free (dst_name);
	}
    }
  else /* !target_directory */
    {
      char const *new_dest;
      char const *source = file[0];
      char const *dest = file[1];
      bool unused;

      if (parents_option)
	{
	  error (0, 0,
		 _("with --parents, the destination must be a directory"));
	  usage (EXIT_FAILURE);
	}

      /* When the force and backup options have been specified and
	 the source and destination are the same name for an existing
	 regular file, convert the user's command, e.g.,
	 `cp --force --backup foo foo' to `cp --force foo fooSUFFIX'
	 where SUFFIX is determined by any version control options used.  */

      if (x->unlink_dest_after_failed_open
	  && x->backup_type != no_backups
	  && STREQ (source, dest)
	  && !new_dst && S_ISREG (sb.st_mode))
	{
	  static struct cp_options x_tmp;

	  new_dest = find_backup_file_name (dest, x->backup_type);
	  /* Set x->backup_type to `no_backups' so that the normal backup
	     mechanism is not used when performing the actual copy.
	     backup_type must be set to `no_backups' only *after* the above
	     call to find_backup_file_name -- that function uses
	     backup_type to determine the suffix it applies.  */
	  x_tmp = *x;
	  x_tmp.backup_type = no_backups;
	  x = &x_tmp;
	}
      else
	{
	  new_dest = dest;
	}

      ok = copy (source, new_dest, 0, x, &unused, NULL);
    }

  return ok;
}

static void
cp_option_init (struct cp_options *x)
{
  x->copy_as_regular = true;
  x->dereference = DEREF_UNDEFINED;
  x->unlink_dest_before_opening = false;
  x->unlink_dest_after_failed_open = false;
  x->hard_link = false;
  x->interactive = I_UNSPECIFIED;
  x->chown_privileges = chown_privileges ();
  x->move_mode = false;
  x->one_file_system = false;

  x->preserve_ownership = false;
  x->preserve_links = false;
  x->preserve_mode = false;
  x->preserve_timestamps = false;

  x->require_preserve = false;
  x->recursive = false;
  x->sparse_mode = SPARSE_AUTO;
  x->symbolic_link = false;
  x->set_mode = false;
  x->mode = 0;

  /* Not used.  */
  x->stdin_tty = false;

  /* Find out the current file creation mask, to knock the right bits
     when using chmod.  The creation mask is set to be liberal, so
     that created directories can be written, even if it would not
     have been allowed with the mask this process was started with.  */
  x->umask_kill = ~ umask (0);

  x->update = false;
  x->verbose = false;
  x->dest_info = NULL;
  x->src_info = NULL;
}

/* Given a string, ARG, containing a comma-separated list of arguments
   to the --preserve option, set the appropriate fields of X to ON_OFF.  */
static void
decode_preserve_arg (char const *arg, struct cp_options *x, bool on_off)
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
      "ownership", "links", "all", NULL
    };
  ARGMATCH_VERIFY (preserve_args, preserve_vals);

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
  bool ok;
  bool make_backups = false;
  char *backup_suffix_string;
  char *version_control_string = NULL;
  struct cp_options x;
  bool copy_contents = false;
  char *target_directory = NULL;
  bool no_target_directory = false;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  cp_option_init (&x);

  /* FIXME: consider not calling getenv for SIMPLE_BACKUP_SUFFIX unless
     we'll actually use backup_suffix_string.  */
  backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  while ((c = getopt_long (argc, argv, "abdfHilLprst:uvxPRS:T",
			   long_opts, NULL))
	 != -1)
    {
      switch (c)
	{
	case SPARSE_OPTION:
	  x.sparse_mode = XARGMATCH ("--sparse", optarg,
				     sparse_type_string, sparse_type);
	  break;

	case 'a':		/* Like -dpPR. */
	  x.dereference = DEREF_NEVER;
	  x.preserve_links = true;
	  x.preserve_ownership = true;
	  x.preserve_mode = true;
	  x.preserve_timestamps = true;
	  x.require_preserve = true;
	  x.recursive = true;
	  break;

	case 'b':
	  make_backups = true;
	  if (optarg)
	    version_control_string = optarg;
	  break;

	case COPY_CONTENTS_OPTION:
	  copy_contents = true;
	  break;

	case 'd':
	  x.preserve_links = true;
	  x.dereference = DEREF_NEVER;
	  break;

	case 'f':
	  x.unlink_dest_after_failed_open = true;
	  break;

	case 'H':
	  x.dereference = DEREF_COMMAND_LINE_ARGUMENTS;
	  break;

	case 'i':
	  x.interactive = I_ASK_USER;
	  break;

	case 'l':
	  x.hard_link = true;
	  break;

	case 'L':
	  x.dereference = DEREF_ALWAYS;
	  break;

	case 'P':
	  x.dereference = DEREF_NEVER;
	  break;

	case NO_PRESERVE_ATTRIBUTES_OPTION:
	  decode_preserve_arg (optarg, &x, false);
	  break;

	case PRESERVE_ATTRIBUTES_OPTION:
	  if (optarg == NULL)
	    {
	      /* Fall through to the case for `p' below.  */
	    }
	  else
	    {
	      decode_preserve_arg (optarg, &x, true);
	      x.require_preserve = true;
	      break;
	    }

	case 'p':
	  x.preserve_ownership = true;
	  x.preserve_mode = true;
	  x.preserve_timestamps = true;
	  x.require_preserve = true;
	  break;

	case PARENTS_OPTION:
	  parents_option = true;
	  break;

	case 'r':
	case 'R':
	  x.recursive = true;
	  break;

	case REPLY_OPTION: /* Deprecated */
	  x.interactive = XARGMATCH ("--reply", optarg,
				     reply_args, reply_vals);
	  error (0, 0,
		 _("the --reply option is deprecated; use -i or -f instead"));
	  break;

	case UNLINK_DEST_BEFORE_OPENING:
	  x.unlink_dest_before_opening = true;
	  break;

	case STRIP_TRAILING_SLASHES_OPTION:
	  remove_trailing_slashes = true;
	  break;

	case 's':
#ifdef S_ISLNK
	  x.symbolic_link = true;
#else
	  error (EXIT_FAILURE, 0,
		 _("symbolic links are not supported on this system"));
#endif
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

	case 'u':
	  x.update = true;
	  break;

	case 'v':
	  x.verbose = true;
	  break;

	case 'x':
	  x.one_file_system = true;
	  break;

	case 'S':
	  make_backups = true;
	  backup_suffix_string = optarg;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (x.hard_link & x.symbolic_link)
    {
      error (0, 0, _("cannot make both hard and symbolic links"));
      usage (EXIT_FAILURE);
    }

  if (backup_suffix_string)
    simple_backup_suffix = xstrdup (backup_suffix_string);

  x.backup_type = (make_backups
		   ? xget_version (_("backup type"),
				   version_control_string)
		   : no_backups);

  if (x.preserve_mode)
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

  if (x.recursive)
    x.copy_as_regular = copy_contents;

  /* If --force (-f) was specified and we're in link-creation mode,
     first remove any existing destination file.  */
  if (x.unlink_dest_after_failed_open & (x.hard_link | x.symbolic_link))
    x.unlink_dest_before_opening = true;

  /* Allocate space for remembering copied and created files.  */

  hash_init ();

  ok = do_copy (argc - optind, argv + optind,
		target_directory, no_target_directory, &x);

  forget_all ();

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
