/* remove.c -- core functions for removing files and directories
   Copyright (C) 88, 90, 91, 1994-2001 Free Software Foundation, Inc.

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

/* Extracted from rm.c and librarified by Jim Meyering.  */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
typedef enum {false = 0, true = 1} bool;
#endif

#include "save-cwd.h"
#include "system.h"
#include "dirname.h"
#include "error.h"
#include "obstack.h"
#include "hash.h"
#include "quote.h"
#include "remove.h"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#ifndef PARAMS
# if defined (__GNUC__) || __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

/* On systems with an lstat function that accepts the empty string,
   arrange to make lstat calls go through the wrapper function.  */
#if HAVE_LSTAT_EMPTY_STRING_BUG
int rpl_lstat PARAMS((const char *, struct stat *));
# define lstat(Name, Stat_buf) rpl_lstat(Name, Stat_buf)
#endif

#ifdef D_INO_IN_DIRENT
# define D_INO(dp) ((dp)->d_ino)
# define ENABLE_CYCLE_CHECK
#else
/* Some systems don't have inodes, so fake them to avoid lots of ifdefs.  */
# define D_INO(dp) 1
#endif

#if !defined S_ISLNK
# define S_ISLNK(Mode) 0
#endif

/* Initial capacity of per-directory hash table of entries that have
   been processed but not been deleted.  */
#define HT_INITIAL_CAPACITY 13

/* Initial capacity of the active directory hash table.  This table will
   be resized only for hierarchies more than about 45 levels deep. */
#define ACTIVE_DIR_INITIAL_CAPACITY 53

int euidaccess ();
int yesno ();

extern char *program_name;

/* state initialized by remove_init, freed by remove_fini  */

/* An entry in the active_dir_map.  */
struct active_dir_ent
{
  ino_t st_ino;
  dev_t st_dev;
  unsigned int depth;
};

/* The name of the directory (starting with and relative to a command
   line argument) being processed.  When a subdirectory is entered, a new
   component is appended (pushed).  When RM chdir's out of a directory,
   the top component is removed (popped).  This is used to form a full
   file name when necessary.  */
static struct obstack dir_stack;

/* Stack of lengths of directory names (including trailing slash)
   appended to dir_stack.  We have to have a separate stack of lengths
   (rather than just popping back to previous slash) because the first
   element pushed onto the dir stack may contain slashes.  */
static struct obstack len_stack;

/* Set of `active' directories from the current command-line argument
   to the level in the hierarchy at which files are being removed.
   A directory is added to the active set when RM begins removing it
   (or its entries), and it is removed from the set just after RM has
   finished processing it.

   This is actually a map (not a set), implemented with a hash table.
   For each active directory, it maps the directory's inode number to the
   depth of that directory relative to the root of the tree being deleted.
   A directory specified on the command line has depth zero.
   This construct is used to detect directory cycles so that RM can warn
   about them rather than iterating endlessly.  */
#ifdef ENABLE_CYCLE_CHECK
static struct hash_table *active_dir_map;
#endif

static inline unsigned int
current_depth (void)
{
  return obstack_object_size (&len_stack) / sizeof (size_t);
}

static void
print_nth_dir (FILE *stream, unsigned int depth)
{
  size_t *length = (size_t *) obstack_base (&len_stack);
  char *dir_name = (char *) obstack_base (&dir_stack);
  unsigned int sum = 0;
  unsigned int i;

  assert (depth < current_depth ());

  for (i = 0; i <= depth; i++)
    {
      sum += length[i];
    }

  fwrite (dir_name, 1, sum - 1, stream);
}

static inline struct active_dir_ent *
make_active_dir_ent (ino_t inum, dev_t device, unsigned int depth)
{
  struct active_dir_ent *ent;
  ent = (struct active_dir_ent *) xmalloc (sizeof *ent);
  ent->st_ino = inum;
  ent->st_dev = device;
  ent->depth = depth;
  return ent;
}

static unsigned int
hash_active_dir_ent (void const *x, unsigned int table_size)
{
  struct active_dir_ent const *ade = x;

  /* Ignoring the device number here should be fine.  */
  return ade->st_ino % table_size;
}

static bool
hash_compare_active_dir_ents (void const *x, void const *y)
{
  struct active_dir_ent const *a = x;
  struct active_dir_ent const *b = y;
  return SAME_INODE (*a, *b) ? true : false;
}

/* A hash function for null-terminated char* strings using
   the method described in Aho, Sethi, & Ullman, p 436. */

static unsigned int
hash_pjw (const void *x, unsigned int tablesize)
{
  const char *s = x;
  unsigned int h = 0;
  unsigned int g;

  while (*s != 0)
    {
      h = (h << 4) + *s++;
      if ((g = h & (unsigned int) 0xf0000000) != 0)
        h = (h ^ (g >> 24)) ^ g;
    }

  return (h % tablesize);
}

static bool
hash_compare_strings (void const *x, void const *y)
{
  return STREQ (x, y) ? true : false;
}

static inline void
push_dir (const char *dir_name)
{
  size_t len;

  len = strlen (dir_name);

  /* Append the string onto the stack.  */
  obstack_grow (&dir_stack, dir_name, len);

  /* Append a trailing slash.  */
  obstack_1grow (&dir_stack, '/');

  /* Add one for the slash.  */
  ++len;

  /* Push the length (including slash) onto its stack.  */
  obstack_grow (&len_stack, &len, sizeof (len));
}

static inline void
pop_dir (void)
{
  int n_lengths = obstack_object_size (&len_stack) / sizeof (size_t);
  size_t *length = (size_t *) obstack_base (&len_stack);
  size_t top_len;

  assert (n_lengths > 0);
  top_len = length[n_lengths - 1];
  assert (top_len >= 2);

  /* Pop off the specified length of pathname.  */
  assert (obstack_object_size (&dir_stack) >= top_len);
  obstack_blank (&dir_stack, -top_len);

  /* Pop the length stack, too.  */
  assert (obstack_object_size (&len_stack) >= sizeof (size_t));
  obstack_blank (&len_stack, (int) -(sizeof (size_t)));
}

/* Copy the SRC_LEN bytes of data beginning at SRC into the DST_LEN-byte
   buffer, DST, so that the last source byte is at the end of the destination
   buffer.  If SRC_LEN is longer than DST_LEN, then set *TRUNCATED to non-zero.
   Set *RESULT to point to the beginning of (the portion of) the source data
   in DST.  Return the number of bytes remaining in the destination buffer.  */

static size_t
right_justify (char *dst, size_t dst_len, const char *src, size_t src_len,
	       char **result, int *truncated)
{
  const char *sp;
  char *dp;

  if (src_len <= dst_len)
    {
      sp = src;
      dp = dst + (dst_len - src_len);
      *truncated = 0;
    }
  else
    {
      sp = src + (src_len - dst_len);
      dp = dst;
      src_len = dst_len;
      *truncated = 1;
    }

  *result = memcpy (dp, sp, src_len);
  return dst_len - src_len;
}

/* Using the global directory name obstack, create the full path to FILENAME.
   Return it in sometimes-realloc'd space that should not be freed by the
   caller.  Realloc as necessary.  If realloc fails, use a static buffer
   and put as long a suffix in that buffer as possible.  */

static char *
full_filename (const char *filename)
{
  static char *buf = NULL;
  static size_t n_allocated = 0;

  int dir_len = obstack_object_size (&dir_stack);
  char *dir_name = (char *) obstack_base (&dir_stack);
  size_t n_bytes_needed;
  size_t filename_len;

  filename_len = strlen (filename);
  n_bytes_needed = dir_len + filename_len + 1;

  if (n_bytes_needed > n_allocated)
    {
      /* This code requires that realloc accept NULL as the first arg.
         This function must not use xrealloc.  Otherwise, an out-of-memory
	 error involving a file name to be expanded here wouldn't ever
	 be issued.  Use realloc and fall back on using a static buffer
	 if memory allocation fails.  */
      buf = realloc (buf, n_bytes_needed);
      n_allocated = n_bytes_needed;

      if (buf == NULL)
	{
#define SBUF_SIZE 512
#define ELLIPSES_PREFIX "[...]"
	  static char static_buf[SBUF_SIZE];
	  int truncated;
	  size_t len;
	  char *p;

	  len = right_justify (static_buf, SBUF_SIZE, filename,
			       filename_len + 1, &p, &truncated);
	  right_justify (static_buf, len, dir_name, dir_len, &p, &truncated);
	  if (truncated)
	    {
	      memcpy (static_buf, ELLIPSES_PREFIX,
		      sizeof (ELLIPSES_PREFIX) - 1);
	    }
	  return p;
	}
    }

  /* Copy directory part, including trailing slash, and then
     append the filename part, including a trailing zero byte.  */
  memcpy (mempcpy (buf, dir_name, dir_len), filename, filename_len + 1);

  assert (strlen (buf) + 1 == n_bytes_needed);

  return buf;
}

static inline void
fspec_init_common (struct File_spec *fs)
{
  fs->have_full_mode = 0;
  fs->have_filetype_mode = 0;
  fs->have_device = 0;
}

void
fspec_init_file (struct File_spec *fs, const char *filename)
{
  fs->filename = (char *) filename;
  fspec_init_common (fs);
}

static inline void
fspec_init_dp (struct File_spec *fs, struct dirent *dp)
{
  fs->filename = dp->d_name;
  fspec_init_common (fs);
  fs->st_ino = D_INO (dp);

#if D_TYPE_IN_DIRENT && defined DT_UNKNOWN && defined DTTOIF
  if (dp->d_type != DT_UNKNOWN)
    {
      fs->have_filetype_mode = 1;
      fs->mode = DTTOIF (dp->d_type);
    }
#endif
}

static inline int
fspec_get_full_mode (struct File_spec *fs)
{
  struct stat stat_buf;

  if (fs->have_full_mode)
    return 0;

  if (lstat (fs->filename, &stat_buf))
    return 1;

  fs->have_full_mode = 1;
  fs->have_filetype_mode = 1;
  fs->mode = stat_buf.st_mode;
  fs->st_ino = stat_buf.st_ino;
  fs->have_device = 1;
  fs->st_dev = stat_buf.st_dev;

  return 0;
}

static inline int
fspec_get_device_number (struct File_spec *fs)
{
  struct stat stat_buf;

  if (fs->have_device)
    return 0;

  if (lstat (fs->filename, &stat_buf))
    return 1;

  fs->have_full_mode = 1;
  fs->have_filetype_mode = 1;
  fs->mode = stat_buf.st_mode;
  fs->st_ino = stat_buf.st_ino;
  fs->have_device = 1;
  fs->st_dev = stat_buf.st_dev;

  return 0;
}

static inline int
fspec_get_filetype_mode (struct File_spec *fs, mode_t *filetype_mode)
{
  int fail;

  fail = fs->have_filetype_mode ? 0 : fspec_get_full_mode (fs);
  if (!fail)
    *filetype_mode = fs->mode;

  return fail;
}

static inline mode_t
fspec_filetype_mode (const struct File_spec *fs)
{
  assert (fs->have_filetype_mode);
  return fs->mode;
}

static int
same_file (const char *file_1, const char *file_2)
{
  struct stat sb1, sb2;
  return (lstat (file_1, &sb1) == 0
	  && lstat (file_2, &sb2) == 0
	  && SAME_INODE (sb1, sb2));
}


/* Recursively remove all of the entries in the current directory.
   Return an indication of the success of the operation.  */

static enum RM_status
remove_cwd_entries (const struct rm_options *x)
{
  /* NOTE: this is static.  */
  static DIR *dirp = NULL;

  /* NULL or a malloc'd and initialized hash table of entries in the
     current directory that have been processed but not removed --
     due either to an error or to an interactive `no' response.  */
  struct hash_table *ht = NULL;

  /* FIXME: describe */
  static struct obstack entry_name_pool;
  static int first_call = 1;

  enum RM_status status = RM_OK;

  if (first_call)
    {
      first_call = 0;
      obstack_init (&entry_name_pool);
    }

  if (dirp)
    {
      if (CLOSEDIR (dirp))
	{
	  /* FIXME-someday: but this is actually the previously opened dir.  */
	  error (0, errno, "%s", quote (full_filename (".")));
	  status = RM_ERROR;
	}
      dirp = NULL;
    }

  do
    {
      /* FIXME: why do this?  */
      errno = 0;

      dirp = opendir (".");
      if (dirp == NULL)
	{
	  if (errno != ENOENT || !x->ignore_missing_files)
	    {
	      error (0, errno, _("cannot open directory %s"),
		     quote (full_filename (".")));
	      status = RM_ERROR;
	    }
	  break;
	}

      while (1)
	{
	  char *entry_name;
	  struct File_spec fs;
	  enum RM_status tmp_status;
	  struct dirent *dp;

/* FILE should be skipped if it is `.' or `..', or if it is in
   the table, HT, of entries we've already processed.  */
#define SKIPPABLE(Ht, File) \
  (DOT_OR_DOTDOT(File) || (Ht && hash_lookup (Ht, File)))

	  /* FIXME: use readdir_r directly into an obstack to avoid
	     the obstack_copy0 below --
	     Suggestion from Uli.  Be careful -- there are different
	     prototypes on e.g. Solaris.

	     Do something like this:
	     #define NAME_MAX_FOR(Parent_dir) pathconf ((Parent_dir),
	                                                 _PC_NAME_MAX);
	     dp = obstack_alloc (sizeof (struct dirent)
	                         + NAME_MAX_FOR (".") + 1);
	     fail = xreaddir (dirp, dp);
	     where xreaddir is ...

	     But what about systems like the hurd where NAME_MAX is supposed
	     to be effectively unlimited.  We don't want to have to allocate
	     a huge buffer to accommodate maximum possible entry name.  */

	  dp = readdir (dirp);

#if ! HAVE_WORKING_READDIR
	  if (dp == NULL)
	    {
	      /* Since we have probably modified the directory since it
		 was opened, readdir returning NULL does not necessarily
		 mean we have read the last entry.  Rewind it and check
		 again.  This happens on SunOS4.1.4 with 254 or more files
		 in a directory.  */
	      rewinddir (dirp);
	      while ((dp = readdir (dirp)) && SKIPPABLE (ht, dp->d_name))
		{
		  /* empty */
		}
	    }
#endif

	  if (dp == NULL)
	    break;

	  if (SKIPPABLE (ht, dp->d_name))
	    continue;

	  fspec_init_dp (&fs, dp);

	  /* Save a copy of the name of this entry, in case we have
	     to add it to the set of unremoved entries below.  */
	  entry_name = obstack_copy0 (&entry_name_pool,
				      dp->d_name, NLENGTH (dp));

	  /* CAUTION: after this call to rm, DP may not be valid --
	     it may have been freed due to a close in a recursive call
	     (through rm and remove_dir) to this function.  */
	  tmp_status = rm (&fs, 0, x);

	  /* Update status.  */
	  if (tmp_status > status)
	    status = tmp_status;
	  assert (VALID_STATUS (status));

	  /* If this entry was not removed (due either to an error or to
	     an interactive `no' response), record it in the hash table so
	     we don't consider it again if we reopen this directory later.  */
	  if (status != RM_OK)
	    {
	      if (ht == NULL)
		{
		  ht = hash_initialize (HT_INITIAL_CAPACITY, NULL, hash_pjw,
					hash_compare_strings, NULL);
		  if (ht == NULL)
		    xalloc_die ();
		}
	      if (! hash_insert (ht, entry_name))
		xalloc_die ();
	    }
	  else
	    {
	      /* This entry was not saved in the hash table.  Free it.  */
	      obstack_free (&entry_name_pool, entry_name);
	    }

	  if (dirp == NULL)
	    break;
	}
    }
  while (dirp == NULL);

  if (dirp)
    {
      if (CLOSEDIR (dirp))
	{
	  error (0, errno, _("closing directory %s"),
		 quote (full_filename (".")));
	  status = RM_ERROR;
	}
      dirp = NULL;
    }

  if (ht)
    {
      hash_free (ht);
    }

  if (obstack_object_size (&entry_name_pool) > 0)
    obstack_free (&entry_name_pool, obstack_base (&entry_name_pool));

  return status;
}

/* Query the user if appropriate, and if ok try to remove the
   file or directory specified by FS.  Return RM_OK if it is removed,
   and RM_ERROR or RM_USER_DECLINED if not.  */

static enum RM_status
remove_file (struct File_spec *fs, const struct rm_options *x)
{
  int asked = 0;
  char *pathname = fs->filename;

  if (!x->ignore_missing_files && (x->interactive || x->stdin_tty)
      && euidaccess (pathname, W_OK))
    {
      if (!S_ISLNK (fspec_filetype_mode (fs)))
	{
	  fprintf (stderr,
		   (S_ISDIR (fspec_filetype_mode (fs))
		    ? _("%s: remove write-protected directory %s? ")
		    : _("%s: remove write-protected file %s? ")),
		   program_name, quote (full_filename (pathname)));
	  if (!yesno ())
	    return RM_USER_DECLINED;

	  asked = 1;
	}
    }

  if (!asked && x->interactive)
    {
      /* FIXME: use a variant of error (instead of fprintf) that doesn't
	 append a newline.  Then we won't have to declare program_name in
	 this file.  */
      fprintf (stderr,
	       (S_ISDIR (fspec_filetype_mode (fs))
		? _("%s: remove directory %s? ")
		: _("%s: remove %s? ")),
	       program_name, quote (full_filename (pathname)));
      if (!yesno ())
	return RM_USER_DECLINED;
    }

  if (x->verbose)
    printf (_("removing %s\n"), quote (full_filename (pathname)));

  if (unlink (pathname) && (errno != ENOENT || !x->ignore_missing_files))
    {
      error (0, errno, _("cannot unlink %s"), quote (full_filename (pathname)));
      return RM_ERROR;
    }
  return RM_OK;
}

/* If not in recursive mode, print an error message and return RM_ERROR.
   Otherwise, query the user if appropriate, then try to recursively
   remove the directory specified by FS.  Return RM_OK if it is removed,
   and RM_ERROR or RM_USER_DECLINED if not.
   FIXME: describe need_save_cwd parameter.  */

static enum RM_status
remove_dir (struct File_spec *fs, int need_save_cwd, const struct rm_options *x)
{
  enum RM_status status;
  struct saved_cwd cwd;
  char *dir_name = fs->filename;
  const char *fmt = NULL;

  if (!x->recursive)
    {
      error (0, 0, _("%s is a directory"), quote (full_filename (dir_name)));
      return RM_ERROR;
    }

  if (!x->ignore_missing_files && (x->interactive || x->stdin_tty)
      && euidaccess (dir_name, W_OK))
    {
      fmt = _("%s: directory %s is write protected; descend into it anyway? ");
    }
  else if (x->interactive)
    {
      fmt = _("%s: descend into directory %s? ");
    }

  if (fmt)
    {
      fprintf (stderr, fmt, program_name, quote (full_filename (dir_name)));
      if (!yesno ())
	return RM_USER_DECLINED;
    }

  if (x->verbose)
    printf (_("removing all entries of directory %s\n"),
	    quote (full_filename (dir_name)));

  /* Save cwd if needed.  */
  if (need_save_cwd && save_cwd (&cwd))
    return RM_ERROR;

  /* Make target directory the current one.  */
  if (chdir (dir_name) < 0)
    {
      error (0, errno, _("cannot change to directory %s"),
	     quote (full_filename (dir_name)));
      if (need_save_cwd)
	free_cwd (&cwd);
      return RM_ERROR;
    }

  /* Verify that the device and inode numbers of `.' are the same as
     the ones we recorded for dir_name before we cd'd into it.  This
     detects the scenario in which an attacker tries to make Bob's rm
     command remove some other directory belonging to Bob.  The method
     would be to replace an existing lstat'd but-not-yet-removed directory
     with a symlink to the target directory.  */
  {
    struct stat sb;
    if (lstat (".", &sb))
      error (EXIT_FAILURE, errno,
	     _("cannot lstat `.' in %s"), quote (full_filename (dir_name)));

    assert (fs->have_device);
    if (!SAME_INODE (sb, *fs))
      {
	error (EXIT_FAILURE, 0,
	       _("ERROR: the directory %s initially had device/inode\n\
numbers %lu/%lu, but now (after a chdir into it), the numbers for `.'\n\
are %lu/%lu.  That means that while rm was running, the directory\n\
was replaced with either another directory or a link to another directory."),
	       quote (full_filename (dir_name)),
	       (unsigned long)(fs->st_dev),
	       (unsigned long)(fs->st_ino),
	       (unsigned long)(sb.st_dev),
	       (unsigned long)(sb.st_ino));
      }
  }

  push_dir (dir_name);

  /* Save a copy of dir_name.  Otherwise, remove_cwd_entries may clobber
     it because it is just a pointer to the dir entry's d_name field, and
     remove_cwd_entries may close the directory.  */
  ASSIGN_STRDUPA (dir_name, dir_name);

  status = remove_cwd_entries (x);

  pop_dir ();

  /* Restore cwd.  */
  if (need_save_cwd)
    {
      if (restore_cwd (&cwd, NULL, NULL))
	{
	  free_cwd (&cwd);
	  return RM_ERROR;
	}
      free_cwd (&cwd);
    }
  else if (chdir ("..") < 0)
    {
      error (0, errno, _("cannot change back to directory %s via `..'"),
	     quote (full_filename (dir_name)));
      return RM_ERROR;
    }

  if (x->interactive)
    {
      fprintf (stderr, _("%s: remove directory %s%s? "),
	       program_name,
	       quote (full_filename (dir_name)),
	       (status != RM_OK ? _(" (might be nonempty)") : ""));
      if (!yesno ())
	{
	  return RM_USER_DECLINED;
	}
    }

  if (x->verbose)
    printf (_("removing the directory itself: %s\n"),
	    quote (full_filename (dir_name)));

  if (rmdir (dir_name) && (errno != ENOENT || !x->ignore_missing_files))
    {
      int saved_errno = errno;

#ifndef EINVAL
# define EINVAL 0
#endif
      /* See if rmdir just failed because DIR_NAME is the current directory.
	 If so, give a better diagnostic than `rm: cannot remove directory
	 `...': Invalid argument'  */
      if (errno == EINVAL && same_file (".", dir_name))
	{
	  error (0, 0, _("cannot remove current directory %s"),
		 quote (full_filename (dir_name)));
	}
      else
	{
	  error (0, saved_errno, _("cannot remove directory %s"),
		 quote (full_filename (dir_name)));
	}
      return RM_ERROR;
    }

  return status;
}

/* Remove the file or directory specified by FS after checking appropriate
   things.  Return RM_OK if it is removed, and RM_ERROR or RM_USER_DECLINED
   if not.  If USER_SPECIFIED_NAME is non-zero, then the name part of FS may
   be `.', `..', or may contain slashes.  Otherwise, it must be a simple file
   name (and hence must specify a file in the current directory).  */

enum RM_status
rm (struct File_spec *fs, int user_specified_name, const struct rm_options *x)
{
  mode_t filetype_mode;

  if (user_specified_name)
    {
      /* CAUTION: this use of base_name works only because any
	 trailing slashes in fs->filename have already been removed. */
      char *base = base_name (fs->filename);

      if (DOT_OR_DOTDOT (base))
	{
	  error (0, 0, _("cannot remove `.' or `..'"));
	  return RM_ERROR;
	}
    }

  if (fspec_get_filetype_mode (fs, &filetype_mode))
    {
      if (x->ignore_missing_files && errno == ENOENT)
	return RM_OK;

      error (0, errno, _("cannot remove %s"),
	     quote (full_filename (fs->filename)));
      return RM_ERROR;
    }

#ifdef ENABLE_CYCLE_CHECK
  if (S_ISDIR (filetype_mode))
    {
      struct active_dir_ent *old_ent;
      struct active_dir_ent *new_ent;

      /* If there is already a directory in the map with the same device
	 and inode numbers, then there is a directory cycle.  */

      if (fspec_get_device_number (fs))
	{
	  error (0, errno, _("cannot stat %s"),
		 quote (full_filename (fs->filename)));
	  return RM_ERROR;
	}
      new_ent = make_active_dir_ent (fs->st_ino, fs->st_dev, current_depth ());
      old_ent = hash_lookup (active_dir_map, new_ent);
      if (old_ent)
	{
	  error (0, 0, _("\
WARNING: Circular directory structure.\n\
This almost certainly means that you have a corrupted file system.\n\
NOTIFY YOUR SYSTEM MANAGER.\n\
The following two directories have the same inode number:\n"));
	  print_nth_dir (stderr, old_ent->depth);
	  fprintf (stderr, "\n%s\n", quote (full_filename (fs->filename)));
	  fflush (stderr);

	  if (x->interactive)
	    {
	      error (0, 0, _("continue? "));
	      if (yesno ())
		return RM_ERROR;
	    }
	  exit (1);
	}

      /* Put this directory in the active_dir_map.  */
      if (! hash_insert (active_dir_map, new_ent))
	xalloc_die ();
    }
#endif

  if (!S_ISDIR (filetype_mode) || x->unlink_dirs)
    {
      return remove_file (fs, x);
    }
  else
    {
      int need_save_cwd = user_specified_name;
      enum RM_status status;

      if (need_save_cwd)
	need_save_cwd = (strchr (fs->filename, '/') != NULL);

      status = remove_dir (fs, need_save_cwd, x);

#ifdef ENABLE_CYCLE_CHECK
      {
	struct active_dir_ent tmp;
	struct active_dir_ent *old_ent;

	/* Remove this directory from the active_dir_map.  */
	tmp.st_ino = fs->st_ino;
	assert (fs->have_device);
	tmp.st_dev = fs->st_dev;
	old_ent = hash_delete (active_dir_map, &tmp);
	assert (old_ent != NULL);
	free (old_ent);
      }
#endif

      return status;
    }
}

void
remove_init (void)
{
  /* Initialize dir-stack obstacks.  */
  obstack_init (&dir_stack);
  obstack_init (&len_stack);

#ifdef ENABLE_CYCLE_CHECK
  active_dir_map = hash_initialize (ACTIVE_DIR_INITIAL_CAPACITY, NULL,
				    hash_active_dir_ent,
				    hash_compare_active_dir_ents, free);
#endif
}

void
remove_fini (void)
{
#ifdef ENABLE_CYCLE_CHECK
  hash_free (active_dir_map);
#endif

  obstack_free (&dir_stack, NULL);
  obstack_free (&len_stack, NULL);
}
