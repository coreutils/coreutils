/* remove.c -- core functions for removing files and directories
   Copyright (C) 88, 90, 91, 1994-2005 Free Software Foundation, Inc.

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

/* Extracted from rm.c and librarified, then rewritten by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <setjmp.h>
#include <assert.h>

#include "save-cwd.h"
#include "system.h"
#include "cycle-check.h"
#include "dirname.h"
#include "error.h"
#include "euidaccess.h"
#include "file-type.h"
#include "hash.h"
#include "hash-pjw.h"
#include "lstat.h"
#include "obstack.h"
#include "quote.h"
#include "remove.h"
#include "root-dev-ino.h"
#include "unlinkdir.h"
#include "yesno.h"

/* Avoid shadowing warnings because these are functions declared
   in dirname.h as well as locals used below.  */
#define dir_name rm_dir_name
#define dir_len rm_dir_len

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

/* This is the maximum number of consecutive readdir/unlink calls that
   can be made (with no intervening rewinddir or closedir/opendir)
   before triggering a bug that makes readdir return NULL even though
   some directory entries have not been processed.  The bug afflicts
   SunOS's readdir when applied to ufs file systems and Darwin 6.5's
   (and OSX v.10.3.8's) HFS+.  This maximum is conservative in that
   demonstrating the problem seems to require a directory containing
   at least 254 deletable entries (which doesn't count . and ..), so
   we could conceivably increase the maximum value to 254.  */
enum
  {
    CONSECUTIVE_READDIR_UNLINK_THRESHOLD = 200
  };

enum Ternary
  {
    T_UNKNOWN = 2,
    T_NO,
    T_YES
  };
typedef enum Ternary Ternary;

/* The prompt function may be called twice for a given directory.
   The first time, we ask whether to descend into it, and the
   second time, we ask whether to remove it.  */
enum Prompt_action
  {
    PA_DESCEND_INTO_DIR = 2,
    PA_REMOVE_DIR
  };

/* Initial capacity of per-directory hash table of entries that have
   been processed but not been deleted.  */
#define HT_UNREMOVABLE_INITIAL_CAPACITY 13

/* An entry in the active directory stack.
   Each entry corresponds to an `active' directory.  */
struct AD_ent
{
  /* For a given active directory, this is the set of names of
     entries in that directory that could/should not be removed.
     For example, `.' and `..', as well as files/dirs for which
     unlink/rmdir failed e.g., due to access restrictions.  */
  Hash_table *unremovable;

  /* Record the status for a given active directory; we need to know
     whether an entry was not removed, either because of an error or
     because the user declined.  */
  enum RM_status status;

  /* The directory's dev/ino.  Used to ensure that `chdir some-subdir', then
     `chdir ..' takes us back to the same directory from which we started).
     (valid for all but the bottommost entry on the stack.  */
  struct dev_ino dev_ino;
};

extern char *program_name;

struct dirstack_state
{
  /* The name of the directory (starting with and relative to a command
     line argument) being processed.  When a subdirectory is entered, a new
     component is appended (pushed).  Remove (pop) the top component
     upon chdir'ing out of a directory.  This is used to form the full
     name of the current directory or a file therein, when necessary.  */
  struct obstack dir_stack;

  /* Stack of lengths of directory names (including trailing slash)
     appended to dir_stack.  We have to have a separate stack of lengths
     (rather than just popping back to previous slash) because the first
     element pushed onto the dir stack may contain slashes.  */
  struct obstack len_stack;

  /* Stack of active directory entries.
     The first `active' directory is the initial working directory.
     Additional active dirs are pushed onto the stack as we `chdir'
     into each directory to be processed.  When finished with the
     hierarchy under a directory, pop the active dir stack.  */
  struct obstack Active_dir;

  /* Used to detect cycles.  */
  struct cycle_check_state cycle_check_state;

  /* Target of a longjmp in case rm has to stop processing the current
     command-line argument.  This happens 1) when rm detects a directory
     cycle or 2) when it has processed one or more directories, but then
     is unable to return to the initial working directory to process
     additional `.'-relative command-line arguments.  */
  jmp_buf current_arg_jumpbuf;
};
typedef struct dirstack_state Dirstack_state;

struct cwd_state
{
  /* The value of errno after a failed save_cwd or restore_cwd.  */
  int saved_errno;

  /* Information (open file descriptor or absolute directory name)
     required in order to restore the initial working directory.  */
  struct saved_cwd saved_cwd;
};

static Dirstack_state *
ds_init (void)
{
  Dirstack_state *ds = xmalloc (sizeof *ds);
  obstack_init (&ds->dir_stack);
  obstack_init (&ds->len_stack);
  obstack_init (&ds->Active_dir);
  return ds;
}

static void
ds_free (Dirstack_state *ds)
{
  obstack_free (&ds->dir_stack, NULL);
  obstack_free (&ds->len_stack, NULL);
  obstack_free (&ds->Active_dir, NULL);
  free (ds);
}

static void
hash_freer (void *x)
{
  free (x);
}

static bool
hash_compare_strings (void const *x, void const *y)
{
  return STREQ (x, y) ? true : false;
}

static inline void
push_dir (Dirstack_state *ds, const char *dir_name)
{
  size_t len = strlen (dir_name);

  /* Append the string onto the stack.  */
  obstack_grow (&ds->dir_stack, dir_name, len);

  /* Append a trailing slash.  */
  obstack_1grow (&ds->dir_stack, '/');

  /* Add one for the slash.  */
  ++len;

  /* Push the length (including slash) onto its stack.  */
  obstack_grow (&ds->len_stack, &len, sizeof (len));
}

/* Return the entry name of the directory on the top of the stack
   in malloc'd storage.  */
static inline char *
top_dir (Dirstack_state const *ds)
{
  size_t n_lengths = obstack_object_size (&ds->len_stack) / sizeof (size_t);
  size_t *length = obstack_base (&ds->len_stack);
  size_t top_len = length[n_lengths - 1];
  char const *p = obstack_next_free (&ds->dir_stack) - top_len;
  char *q = xmalloc (top_len);
  memcpy (q, p, top_len - 1);
  q[top_len - 1] = 0;
  return q;
}

static inline void
pop_dir (Dirstack_state *ds)
{
  size_t n_lengths = obstack_object_size (&ds->len_stack) / sizeof (size_t);
  size_t *length = obstack_base (&ds->len_stack);
  size_t top_len;

  assert (n_lengths > 0);
  top_len = length[n_lengths - 1];
  assert (top_len >= 2);

  /* Pop the specified length of file name.  */
  assert (obstack_object_size (&ds->dir_stack) >= top_len);
  obstack_blank (&ds->dir_stack, -top_len);

  /* Pop the length stack, too.  */
  assert (obstack_object_size (&ds->len_stack) >= sizeof (size_t));
  obstack_blank (&ds->len_stack, -(int) sizeof (size_t));
}

/* Copy the SRC_LEN bytes of data beginning at SRC into the DST_LEN-byte
   buffer, DST, so that the last source byte is at the end of the destination
   buffer.  If SRC_LEN is longer than DST_LEN, then set *TRUNCATED.
   Set *RESULT to point to the beginning of (the portion of) the source data
   in DST.  Return the number of bytes remaining in the destination buffer.  */

static size_t
right_justify (char *dst, size_t dst_len, const char *src, size_t src_len,
	       char **result, bool *truncated)
{
  const char *sp;
  char *dp;

  if (src_len <= dst_len)
    {
      sp = src;
      dp = dst + (dst_len - src_len);
      *truncated = false;
    }
  else
    {
      sp = src + (src_len - dst_len);
      dp = dst;
      src_len = dst_len;
      *truncated = true;
    }

  *result = memcpy (dp, sp, src_len);
  return dst_len - src_len;
}

/* Using the global directory name obstack, create the full name FILENAME.
   Return it in sometimes-realloc'd space that should not be freed by the
   caller.  Realloc as necessary.  If realloc fails, use a static buffer
   and put as long a suffix in that buffer as possible.  */

#define full_filename(Filename) full_filename_ (ds, Filename)
static char *
full_filename_ (Dirstack_state const *ds, const char *filename)
{
  static char *buf = NULL;
  static size_t n_allocated = 0;

  size_t dir_len = obstack_object_size (&ds->dir_stack);
  char *dir_name = obstack_base (&ds->dir_stack);
  size_t n_bytes_needed;
  size_t filename_len;

  filename_len = strlen (filename);
  n_bytes_needed = dir_len + filename_len + 1;

  if (n_allocated < n_bytes_needed)
    {
      /* This code requires that realloc accept NULL as the first arg.
         This function must not use xrealloc.  Otherwise, an out-of-memory
	 error involving a file name to be expanded here wouldn't ever
	 be issued.  Use realloc and fall back on using a static buffer
	 if memory allocation fails.  */
      char *new_buf = realloc (buf, n_bytes_needed);
      n_allocated = n_bytes_needed;

      if (new_buf == NULL)
	{
#define SBUF_SIZE 512
#define ELLIPSES_PREFIX "[...]"
	  static char static_buf[SBUF_SIZE];
	  bool truncated;
	  size_t len;
	  char *p;

	  free (buf);
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

      buf = new_buf;
    }

  if (filename_len == 1 && *filename == '.' && dir_len)
    {
      /* FILENAME is just `.' and dir_len is nonzero.
	 Copy the directory part, omitting the trailing slash,
	 and append a trailing zero byte.  */
      char *p = mempcpy (buf, dir_name, dir_len - 1);
      *p = 0;
    }
  else
    {
      /* Copy the directory part, including trailing slash, and then
	 append the filename part, including a trailing zero byte.  */
      memcpy (mempcpy (buf, dir_name, dir_len), filename, filename_len + 1);
      assert (strlen (buf) + 1 == n_bytes_needed);
    }

  return buf;
}

static size_t
AD_stack_height (Dirstack_state const *ds)
{
  return obstack_object_size (&ds->Active_dir) / sizeof (struct AD_ent);
}

static struct AD_ent *
AD_stack_top (Dirstack_state const *ds)
{
  return (struct AD_ent *)
    ((char *) obstack_next_free (&ds->Active_dir) - sizeof (struct AD_ent));
}

static void
AD_stack_pop (Dirstack_state *ds)
{
  /* operate on Active_dir.  pop and free top entry */
  struct AD_ent *top = AD_stack_top (ds);
  if (top->unremovable)
    hash_free (top->unremovable);
  obstack_blank (&ds->Active_dir, -(int) sizeof (struct AD_ent));
  pop_dir (ds);
}

/* chdir `up' one level.
   Whenever using chdir '..', verify that the post-chdir
   dev/ino numbers for `.' match the saved ones.
   If they don't match, exit nonzero.
   Set *PREV_DIR to the name (in malloc'd storage) of the
   directory (usually now empty) from which we're coming.
   If we're at the bottom of the AD stack (about to return
   to the initial working directory), then use CWD.
   If that restore_cwd fails, set CWD_STATE->saved_errno.  */
static void
AD_pop_and_chdir (Dirstack_state *ds, char **prev_dir,
		  struct cwd_state *cwd_state)
{
  enum RM_status old_status = AD_stack_top(ds)->status;
  struct AD_ent *top;

  /* Get the name of the current (but soon to be `previous') directory
     from the top of the stack.  */
  *prev_dir = top_dir (ds);

  AD_stack_pop (ds);
  top = AD_stack_top (ds);

  /* Propagate any failure to parent.  */
  UPDATE_STATUS (top->status, old_status);

  assert (AD_stack_height (ds));

  if (1 < AD_stack_height (ds))
    {
      struct stat sb;

      /* We can give a better diagnostic here, since the target is relative. */
      if (chdir ("..") != 0)
	{
	  error (EXIT_FAILURE, errno,
		 _("cannot chdir from %s to .."),
		 quote (full_filename (".")));
	}

      if (lstat (".", &sb))
	error (EXIT_FAILURE, errno,
	       _("cannot lstat `.' in %s"), quote (full_filename (".")));

      /*  Ensure that post-chdir dev/ino match the stored ones.  */
      if ( ! SAME_INODE (sb, top->dev_ino))
	error (EXIT_FAILURE, 0,
	       _("%s changed dev/ino"), quote (full_filename (".")));
    }
  else
    {
      if (restore_cwd (&cwd_state->saved_cwd) != 0)
	{
	  /* We've failed to return to the initial working directory.
	     That failure may be harmless if x->require_restore_cwd is false,
	     but we do have to remember that fact, including the errno value,
	     so we can give an accurate diagnostic when reporting the failure
	     to remove a subsequent relative-named command-line argument.  */
	  cwd_state->saved_errno = errno;
	}
    }
}

/* Initialize *HT if it is NULL.
   Insert FILENAME into HT.  */
static void
AD_mark_helper (Hash_table **ht, char *filename)
{
  if (*ht == NULL)
    {
      *ht = hash_initialize (HT_UNREMOVABLE_INITIAL_CAPACITY, NULL, hash_pjw,
			     hash_compare_strings, hash_freer);
      if (*ht == NULL)
	xalloc_die ();
    }
  if (! hash_insert (*ht, filename))
    xalloc_die ();
}

/* Mark FILENAME (in current directory) as unremovable.  */
static void
AD_mark_as_unremovable (Dirstack_state *ds, char const *filename)
{
  AD_mark_helper (&AD_stack_top(ds)->unremovable, xstrdup (filename));
}

/* Mark the current directory as unremovable.  I.e., mark the entry
   in the parent directory corresponding to `.'.
   This happens e.g., when an opendir fails and the only name
   the caller has conveniently at hand is `.'.  */
static void
AD_mark_current_as_unremovable (Dirstack_state *ds)
{
  struct AD_ent *top = AD_stack_top (ds);
  char *curr = top_dir (ds);

  assert (1 < AD_stack_height (ds));

  --top;
  AD_mark_helper (&top->unremovable, curr);
}

/* Push an initial dummy entry onto the stack.
   This will always be the bottommost entry on the stack.  */
static void
AD_push_initial (Dirstack_state *ds)
{
  struct AD_ent *top;

  /* Extend the stack.  */
  obstack_blank (&ds->Active_dir, sizeof (struct AD_ent));

  /* Fill in the new values.  */
  top = AD_stack_top (ds);
  top->unremovable = NULL;

  /* These should never be used.
     Give them values that might look suspicious
     in a debugger or in a diagnostic.  */
  top->dev_ino.st_dev = TYPE_MAXIMUM (dev_t);
  top->dev_ino.st_ino = TYPE_MAXIMUM (ino_t);
}

/* Push info about the current working directory (".") onto the
   active directory stack.  DIR is the ./-relative name through
   which we've just `chdir'd to this directory.  DIR_SB_FROM_PARENT
   is the result of calling lstat on DIR from the parent of DIR.  */
static void
AD_push (Dirstack_state *ds, char const *dir,
	 struct stat const *dir_sb_from_parent)
{
  struct stat sb;
  struct AD_ent *top;

  push_dir (ds, dir);

  if (lstat (".", &sb))
    error (EXIT_FAILURE, errno,
	   _("cannot lstat `.' in %s"), quote (full_filename (".")));

  if ( ! SAME_INODE (sb, *dir_sb_from_parent))
    error (EXIT_FAILURE, 0,
	   _("%s changed dev/ino"), quote (full_filename (".")));

  /* Extend the stack.  */
  obstack_blank (&ds->Active_dir, sizeof (struct AD_ent));

  /* Fill in the new values.  */
  top = AD_stack_top (ds);
  top->dev_ino.st_dev = sb.st_dev;
  top->dev_ino.st_ino = sb.st_ino;
  top->unremovable = NULL;
}

static bool
AD_is_removable (Dirstack_state const *ds, char const *file)
{
  struct AD_ent *top = AD_stack_top (ds);
  return ! (top->unremovable && hash_lookup (top->unremovable, file));
}

/* Return true if DIR is determined to be an empty directory
   or if opendir or readdir fails.  */
static bool
is_empty_dir (char const *dir)
{
  DIR *dirp = opendir (dir);
  struct dirent const *dp;
  int saved_errno;

  if (dirp == NULL)
    return false;

  errno = 0;
  dp = readdir_ignoring_dot_and_dotdot (dirp);
  saved_errno = errno;
  closedir (dirp);
  if (dp != NULL)
    return false;
  return saved_errno == 0 ? true : false;
}

/* Return true if FILE is determined to be an unwritable non-symlink.
   Otherwise, return false (including when lstat'ing it fails).
   If lstat succeeds, set *BUF_P to BUF.
   This is to avoid calling euidaccess when FILE is a symlink.  */
static bool
write_protected_non_symlink (char const *file,
			     struct stat **buf_p,
			     struct stat *buf)
{
  if (lstat (file, buf) != 0)
    return false;
  *buf_p = buf;
  if (S_ISLNK (buf->st_mode))
    return false;
  return euidaccess (file, W_OK) == -1 && errno == EACCES;
}

/* Prompt whether to remove FILENAME, if required via a combination of
   the options specified by X and/or file attributes.  If the file may
   be removed, return RM_OK.  If the user declines to remove the file,
   return RM_USER_DECLINED.  If not ignoring missing files and we
   cannot lstat FILENAME, then return RM_ERROR.

   Depending on MODE, ask whether to `descend into' or to `remove' the
   directory FILENAME.  MODE is ignored when FILENAME is not a directory.
   Set *IS_EMPTY to T_YES if FILENAME is an empty directory, and it is
   appropriate to try to remove it with rmdir (e.g. recursive mode).
   Don't even try to set *IS_EMPTY when MODE == PA_REMOVE_DIR.
   Set *IS_DIR to T_YES or T_NO if we happen to determine whether
   FILENAME is a directory.  */
static enum RM_status
prompt (Dirstack_state const *ds, char const *filename,
	struct rm_options const *x, enum Prompt_action mode,
	Ternary *is_dir, Ternary *is_empty)
{
  bool write_protected = false;
  struct stat *sbuf = NULL;
  struct stat buf;

  *is_empty = T_UNKNOWN;
  *is_dir = T_UNKNOWN;

  if (((!x->ignore_missing_files & (x->interactive | x->stdin_tty))
       && (write_protected = write_protected_non_symlink (filename,
							  &sbuf, &buf)))
      || x->interactive)
    {
      if (sbuf == NULL)
	{
	  sbuf = &buf;
	  if (lstat (filename, sbuf))
	    {
	      /* lstat failed.  This happens e.g., with `rm '''.  */
	      error (0, errno, _("cannot lstat %s"),
		     quote (full_filename (filename)));
	      return RM_ERROR;
	    }
	}

      if (S_ISDIR (sbuf->st_mode) && !x->recursive)
	{
	  error (0, EISDIR, _("cannot remove directory %s"),
		 quote (full_filename (filename)));
	  return RM_ERROR;
	}

      /* Using permissions doesn't make sense for symlinks.  */
      if (S_ISLNK (sbuf->st_mode))
	{
	  if ( ! x->interactive)
	    return RM_OK;
	  write_protected = false;
	}

      /* Issue the prompt.  */
      {
	char const *quoted_name = quote (full_filename (filename));

	*is_dir = (S_ISDIR (sbuf->st_mode) ? T_YES : T_NO);

	/* FIXME: use a variant of error (instead of fprintf) that doesn't
	   append a newline.  Then we won't have to declare program_name in
	   this file.  */
	if (S_ISDIR (sbuf->st_mode)
	    && x->recursive
	    && mode == PA_DESCEND_INTO_DIR
	    && ((*is_empty = (is_empty_dir (filename) ? T_YES : T_NO))
		== T_NO))
	  fprintf (stderr,
		   (write_protected
		    ? _("%s: descend into write-protected directory %s? ")
		    : _("%s: descend into directory %s? ")),
		   program_name, quoted_name);
	else
	  {
	    /* TRANSLATORS: You may find it more convenient to translate
	       the equivalent of _("%s: remove %s (write-protected) %s? ").
	       It should avoid grammatical problems with the output
	       of file_type.  */
	    fprintf (stderr,
		     (write_protected
		      ? _("%s: remove write-protected %s %s? ")
		      : _("%s: remove %s %s? ")),
		     program_name, file_type (sbuf), quoted_name);
	  }

	if (!yesno ())
	  return RM_USER_DECLINED;
      }
    }
  return RM_OK;
}

#if HAVE_STRUCT_DIRENT_D_TYPE

/* True if the type of the directory entry D is known.  */
# define DT_IS_KNOWN(d) ((d)->d_type != DT_UNKNOWN)

/* True if the type of the directory entry D must be T.  */
# define DT_MUST_BE(d, t) ((d)->d_type == (t))

#else
# define DT_IS_KNOWN(d) false
# define DT_MUST_BE(d, t) false
#endif

#define DO_UNLINK(Filename, X)						\
  do									\
    {									\
      if (unlink (Filename) == 0)					\
	{								\
	  if ((X)->verbose)						\
	    printf (_("removed %s\n"), quote (full_filename (Filename))); \
	  return RM_OK;							\
	}								\
									\
      if (errno == ENOENT && (X)->ignore_missing_files)			\
	return RM_OK;							\
    }									\
  while (0)

#define DO_RMDIR(Filename, X)				\
  do							\
    {							\
      if (rmdir (Filename) == 0)			\
	{						\
	  if ((X)->verbose)				\
	    printf (_("removed directory: %s\n"),	\
		    quote (full_filename (Filename)));	\
	  return RM_OK;					\
	}						\
							\
      if (errno == ENOENT && (X)->ignore_missing_files)	\
	return RM_OK;					\
							\
      if (errno == ENOTEMPTY || errno == EEXIST)	\
	return RM_NONEMPTY_DIR;				\
    }							\
  while (0)

/* Remove the file or directory specified by FILENAME.
   Return RM_OK if it is removed, and RM_ERROR or RM_USER_DECLINED if not.
   But if FILENAME specifies a non-empty directory, return RM_NONEMPTY_DIR. */

static enum RM_status
remove_entry (Dirstack_state const *ds, char const *filename,
	      struct rm_options const *x, struct dirent const *dp)
{
  Ternary is_dir;
  Ternary is_empty_directory;
  enum RM_status s = prompt (ds, filename, x, PA_DESCEND_INTO_DIR,
			     &is_dir, &is_empty_directory);

  if (s != RM_OK)
    return s;

  /* Why bother with the following if/else block?  Because on systems with
     an unlink function that *can* unlink directories, we must determine the
     type of each entry before removing it.  Otherwise, we'd risk unlinking
     an entire directory tree simply by unlinking a single directory;  then
     all the storage associated with that hierarchy would not be freed until
     the next fsck.  Not nice.  To avoid that, on such slightly losing
     systems, we need to call lstat to determine the type of each entry,
     and that represents extra overhead that -- it turns out -- we can
     avoid on non-losing systems, since there, unlink will never remove
     a directory.  Also, on systems where unlink may unlink directories,
     we're forced to allow a race condition: we lstat a non-directory, then
     go to unlink it, but in the mean time, a malicious someone has replaced
     it with a directory.  */

  if (cannot_unlink_dir ())
    {
      if (is_dir == T_YES && ! x->recursive)
	{
	  error (0, EISDIR, _("cannot remove directory %s"),
		 quote (full_filename (filename)));
	  return RM_ERROR;
	}

      /* is_empty_directory is set iff it's ok to use rmdir.
	 Note that it's set only in interactive mode -- in which case it's
	 an optimization that arranges so that the user is asked just
	 once whether to remove the directory.  */
      if (is_empty_directory == T_YES)
	DO_RMDIR (filename, x);

      /* If we happen to know that FILENAME is a directory, return now
	 and let the caller remove it -- this saves the overhead of a failed
	 unlink call.  If FILENAME is a command-line argument, then dp is NULL,
	 so we'll first try to unlink it.  Using unlink here is ok, because it
	 cannot remove a directory.  */
      if ((dp && DT_MUST_BE (dp, DT_DIR)) || is_dir == T_YES)
	return RM_NONEMPTY_DIR;

      DO_UNLINK (filename, x);

      if (! x->recursive
	  || errno == ENOENT || errno == ENOTDIR
	  || errno == ELOOP || errno == ENAMETOOLONG)
	{
	  /* Either --recursive is not in effect, or the file cannot be a
	     directory.  Report the unlink problem and fail.  */
	  error (0, errno, _("cannot remove %s"),
		 quote (full_filename (filename)));
	  return RM_ERROR;
	}
    }
  else
    {
      /* If we don't already know whether FILENAME is a directory, find out now.
	 Then, if it's a non-directory, we can use unlink on it.  */
      if (is_dir == T_UNKNOWN)
	{
	  if (dp && DT_IS_KNOWN (dp))
	    is_dir = DT_MUST_BE (dp, DT_DIR) ? T_YES : T_NO;
	  else
	    {
	      struct stat sbuf;
	      if (lstat (filename, &sbuf))
		{
		  if (errno == ENOENT && x->ignore_missing_files)
		    return RM_OK;

		  error (0, errno, _("cannot lstat %s"),
			 quote (full_filename (filename)));
		  return RM_ERROR;
		}

	      is_dir = S_ISDIR (sbuf.st_mode) ? T_YES : T_NO;
	    }
	}

      if (is_dir == T_NO)
	{
	  /* At this point, barring race conditions, FILENAME is known
	     to be a non-directory, so it's ok to try to unlink it.  */
	  DO_UNLINK (filename, x);

	  /* unlink failed with some other error code.  report it.  */
	  error (0, errno, _("cannot remove %s"),
		 quote (full_filename (filename)));
	  return RM_ERROR;
	}

      if (! x->recursive)
	{
	  error (0, EISDIR, _("cannot remove directory %s"),
		 quote (full_filename (filename)));
	  return RM_ERROR;
	}

      if (is_empty_directory == T_YES)
	{
	  DO_RMDIR (filename, x);
	  /* Don't diagnose any failure here.
	     It'll be detected when the caller tries another way.  */
	}
    }

  return RM_NONEMPTY_DIR;
}

/* Remove entries in `.', the current working directory (cwd).
   Upon finding a directory that is both non-empty and that can be chdir'd
   into, return RM_OK and set *SUBDIR and fill in SUBDIR_SB, where
   SUBDIR is the malloc'd name of the subdirectory if the chdir succeeded,
   NULL otherwise (e.g., if opendir failed or if there was no subdirectory).
   Likewise, SUBDIR_SB is the result of calling lstat on SUBDIR.
   Return RM_OK if all entries are removed.  Return RM_ERROR if any
   entry cannot be removed.  Otherwise, return RM_USER_DECLINED if
   the user declines to remove at least one entry.  Remove as much as
   possible, continuing even if we fail to remove some entries.  */
static enum RM_status
remove_cwd_entries (Dirstack_state *ds, char **subdir, struct stat *subdir_sb,
		    struct rm_options const *x)
{
  DIR *dirp = opendir (".");
  struct AD_ent *top = AD_stack_top (ds);
  enum RM_status status = top->status;
  size_t n_unlinked_since_opendir_or_last_rewind = 0;

  assert (VALID_STATUS (status));
  *subdir = NULL;

  if (dirp == NULL)
    {
      if (errno != ENOENT || !x->ignore_missing_files)
	{
	  error (0, errno, _("cannot open directory %s"),
		 quote (full_filename (".")));
	  return RM_ERROR;
	}
    }

  while (1)
    {
      struct dirent const *dp;
      enum RM_status tmp_status;
      const char *f;

      /* Set errno to zero so we can distinguish between a readdir failure
	 and when readdir simply finds that there are no more entries.  */
      errno = 0;
      dp = readdir_ignoring_dot_and_dotdot (dirp);
      if (dp == NULL)
	{
	  if (errno)
	    {
	      /* Save/restore errno across closedir call.  */
	      int e = errno;
	      closedir (dirp);
	      errno = e;

	      /* Arrange to give a diagnostic after exiting this loop.  */
	      dirp = NULL;
	    }
	  else if (CONSECUTIVE_READDIR_UNLINK_THRESHOLD
		   < n_unlinked_since_opendir_or_last_rewind)
	    {
	      /* Call rewinddir if we've called unlink or rmdir so many times
		 (since the opendir or the previous rewinddir) that this
		 NULL-return may be the symptom of a buggy readdir.  */
	      rewinddir (dirp);
	      n_unlinked_since_opendir_or_last_rewind = 0;
	      continue;
	    }
	  break;
	}

      f = dp->d_name;

      /* Skip files we've already tried/failed to remove.  */
      if ( ! AD_is_removable (ds, f))
	continue;

      /* Pass dp->d_type info to remove_entry so the non-glibc
	 case can decide whether to use unlink or chdir.
	 Systems without the d_type member will have to endure
	 the performance hit of first calling lstat F. */
      tmp_status = remove_entry (ds, f, x, dp);
      switch (tmp_status)
	{
	case RM_OK:
	  /* Count how many files we've unlinked since the initial
	     opendir or the last rewinddir.  On buggy systems, if you
	     remove too many, readdir returns NULL even though there
	     remain unprocessed directory entries.  */
	  ++n_unlinked_since_opendir_or_last_rewind;
	  break;

	case RM_ERROR:
	case RM_USER_DECLINED:
	  AD_mark_as_unremovable (ds, f);
	  UPDATE_STATUS (status, tmp_status);
	  break;

	case RM_NONEMPTY_DIR:
	  {
	    /* Save a copy of errno, in case the preceding unlink (from
	       remove_entry's DO_UNLINK) of a non-directory failed.  */
	    int saved_errno = errno;

	    /* Record dev/ino of F so that we can compare
	       that with dev/ino of `.' after the chdir.
	       This dev/ino pair is also used in cycle detection.  */
	    if (lstat (f, subdir_sb))
	      error (EXIT_FAILURE, errno, _("cannot lstat %s"),
		     quote (full_filename (f)));

	    errno = ENOTDIR;
	    if (! S_ISDIR (subdir_sb->st_mode) || chdir (f) != 0)
	      {
		/* It is much more common that we reach this point for an
		   inaccessible directory.  Hence the second diagnostic, below.
		   However it is also possible that F is a non-directory.
		   That can happen when we use the `! UNLINK_CAN_UNLINK_DIRS'
		   block of code and when DO_UNLINK fails due to EPERM.
		   In that case, give a better diagnostic.  */
		if (errno == ENOTDIR)
		  error (0, saved_errno, _("cannot remove %s"),
			 quote (full_filename (f)));
		else
		  error (0, errno, _("cannot chdir from %s to %s"),
			 quote_n (0, full_filename (".")), quote_n (1, f));
		AD_mark_as_unremovable (ds, f);
		status = RM_ERROR;
		break;
	      }
	    if (cycle_check (&ds->cycle_check_state, subdir_sb))
	      {
		error (0, 0, _("\
WARNING: Circular directory structure.\n\
This almost certainly means that you have a corrupted file system.\n\
NOTIFY YOUR SYSTEM MANAGER.\n\
The following directory is part of the cycle:\n  %s\n"),
		       quote (full_filename (".")));
		longjmp (ds->current_arg_jumpbuf, 1);
	      }

	    *subdir = xstrdup (f);
	    break;
	  }
	}

      /* Record status for this directory.  */
      UPDATE_STATUS (top->status, status);

      if (*subdir)
	break;
    }

  if (dirp == NULL || CLOSEDIR (dirp) != 0)
    {
      /* Note that this diagnostic serves for both readdir
	 and closedir failures.  */
      error (0, errno, _("reading directory %s"), quote (full_filename (".")));
      status = RM_ERROR;
    }

  return status;
}

/* Do this after each call to AD_push or AD_push_initial.
   Because the status = RM_OK bit is too remove-specific to
   go into the general-purpose AD_* package.  */
#define AD_INIT_OTHER_MEMBERS()			\
  do						\
    {						\
      AD_stack_top(ds)->status = RM_OK;		\
    }						\
  while (0)

/*  Remove the hierarchy rooted at DIR.
    Do that by changing into DIR, then removing its contents, then
    returning to the original working directory and removing DIR itself.
    Don't use recursion.  Be careful when using chdir ".." that we
    return to the same directory from which we came, if necessary.
    Return 1 for success, 0 if some file cannot be removed or if
    a chdir fails.
    If the initial working directory cannot be saved or restored,
    record the offending errno value in (*CWD_STATE)->saved_errno.  */

static enum RM_status
remove_dir (Dirstack_state *ds, char const *dir, struct cwd_state **cwd_state,
	    struct rm_options const *x)
{
  enum RM_status status;
  struct stat dir_sb;

  /* Save any errno (from caller's failed remove_entry call), in case DIR
     is not a directory, so that we can give a reasonable diagnostic.  */
  int saved_errno = errno;

  if (*cwd_state == NULL)
    {
      *cwd_state = xmalloc (sizeof **cwd_state);

      if (save_cwd (&(*cwd_state)->saved_cwd) != 0)
	{
	  (*cwd_state)->saved_errno = errno;
	  assert (errno != 0);

	  /* Pretend we started from "/".  That is fine as long as there
	     is no requirement to return to the original working directory.
	     Use "/", not ".", so that we chdir out of a non-root target
	     directory before attempting to remove it: some hosts don't let
	     you remove a working directory.  */
	  (*cwd_state)->saved_cwd.name = xstrdup ("/");
	}
      else
	(*cwd_state)->saved_errno = 0;

      AD_push_initial (ds);
      AD_INIT_OTHER_MEMBERS ();
    }

  /* If we've failed to record and/or restore the initial working directory,
     and we're now trying to access a `.'-relative file name, then give a
     diagnostic, record the failure, and proceed with any subsequent
     command-line arguments.  */
  if ((*cwd_state)->saved_errno && IS_RELATIVE_FILE_NAME (dir))
    {
      error (0, (*cwd_state)->saved_errno, _("cannot remove directory %s"),
	     quote (full_filename (dir)));
      longjmp (ds->current_arg_jumpbuf, 1);
    }

  /* There is a race condition in that an attacker could replace the nonempty
     directory, DIR, with a symlink between the preceding call to rmdir
     (in our caller) and the chdir below.  However, the following lstat,
     along with the `stat (".",...' and dev/ino comparison in AD_push
     ensure that we detect it and fail.  */

  if (lstat (dir, &dir_sb))
    {
      error (0, errno,
	     _("cannot lstat %s"), quote (full_filename (dir)));
      return RM_ERROR;
    }

  errno = ENOTDIR;
  if (! S_ISDIR (dir_sb.st_mode) || chdir (dir) != 0)
    {
      if (errno == ENOTDIR)
	{
	  error (0, saved_errno,
		 _("cannot remove %s"), quote (full_filename (dir)));
	}
      else
	{
	  error (0, errno,
		 _("cannot chdir from %s to %s"),
		 quote_n (0, full_filename (".")), quote_n (1, dir));
	}
      return RM_ERROR;
    }

  if (ROOT_DEV_INO_CHECK (x->root_dev_ino, &dir_sb))
    {
      ROOT_DEV_INO_WARN (full_filename (dir));
      return RM_ERROR;
    }

  AD_push (ds, dir, &dir_sb);
  AD_INIT_OTHER_MEMBERS ();

  status = RM_OK;

  while (1)
    {
      char *subdir = NULL;
      struct stat subdir_sb;
      enum RM_status tmp_status = remove_cwd_entries (ds,
						      &subdir, &subdir_sb, x);
      if (tmp_status != RM_OK)
	{
	  UPDATE_STATUS (status, tmp_status);
	  AD_mark_current_as_unremovable (ds);
	}
      if (subdir)
	{
	  AD_push (ds, subdir, &subdir_sb);
	  AD_INIT_OTHER_MEMBERS ();

	  free (subdir);
	  continue;
	}

      /* Execution reaches this point when we've removed the last
	 removable entry from the current directory.  */
      {
	/* This is the name of the directory that we have just
	   returned from, after nominally removing all of its contents.  */
	char *empty_dir;

	AD_pop_and_chdir (ds, &empty_dir, *cwd_state);

	/* Try to remove D only if remove_cwd_entries succeeded.  */
	if (tmp_status == RM_OK)
	  {
	    /* This does a little more work than necessary when it actually
	       prompts the user.  E.g., we already know that D is a directory
	       and that it's almost certainly empty, yet we lstat it.
	       But that's no big deal since we're interactive.  */
	    Ternary is_dir;
	    Ternary is_empty;
	    enum RM_status s = prompt (ds, empty_dir, x, PA_REMOVE_DIR,
				       &is_dir, &is_empty);

	    if (s != RM_OK)
	      {
		free (empty_dir);
		return s;
	      }

	    if (rmdir (empty_dir) == 0)
	      {
		if (x->verbose)
		  printf (_("removed directory: %s\n"),
			  quote (full_filename (empty_dir)));
	      }
	    else
	      {
		error (0, errno, _("cannot remove directory %s"),
		       quote (full_filename (empty_dir)));
		AD_mark_as_unremovable (ds, empty_dir);
		status = RM_ERROR;
		UPDATE_STATUS (AD_stack_top(ds)->status, status);
	      }
	  }

	free (empty_dir);

	if (AD_stack_height (ds) == 1)
	  break;
      }
    }

  return status;
}

/* Remove the file or directory specified by FILENAME.
   Return RM_OK if it is removed, and RM_ERROR or RM_USER_DECLINED if not.
   On input, the first time this function is called, CWD_STATE should be
   the address of a NULL pointer.  Do not modify it for any subsequent calls.
   On output, it is either that same NULL pointer or the address of
   a malloc'd `struct saved_cwd' that may be freed.  */

static enum RM_status
rm_1 (Dirstack_state *ds, char const *filename,
      struct rm_options const *x, struct cwd_state **cwd_state)
{
  char *base = base_name (filename);
  enum RM_status status;

  if (DOT_OR_DOTDOT (base))
    {
      error (0, 0, _("cannot remove `.' or `..'"));
      return RM_ERROR;
    }

  if (*cwd_state && (*cwd_state)->saved_errno
      && IS_RELATIVE_FILE_NAME (filename))
    {
      error (0, (*cwd_state)->saved_errno,
	     _("cannot remove %s"), quote (filename));
      return RM_ERROR;
    }

  status = remove_entry (ds, filename, x, NULL);
  if (status != RM_NONEMPTY_DIR)
    return status;

  return remove_dir (ds, filename, cwd_state, x);
}

/* Remove all files and/or directories specified by N_FILES and FILE.
   Apply the options in X.  If X->require_restore_cwd is false, then
   this function may return RM_OK even though it is unable to restore
   the initial working directory.  */
extern enum RM_status
rm (size_t n_files, char const *const *file, struct rm_options const *x)
{
  struct cwd_state *cwd_state = NULL;
  Dirstack_state *ds;

  /* Put the following two variables in static storage, so they can't
     be clobbered by the potential longjmp into this function.  */
  static enum RM_status status = RM_OK;
  static size_t i;

  ds = ds_init ();

  for (i = 0; i < n_files; i++)
    {
      enum RM_status s;
      cycle_check_init (&ds->cycle_check_state);
      /* In the event that rm_1->remove_dir->remove_cwd_entries detects
	 a directory cycle, arrange to fail, give up on this FILE, but
	 continue on with any other arguments.  */
      if (setjmp (ds->current_arg_jumpbuf))
	s = RM_ERROR;
      else
	s = rm_1 (ds, file[i], x, &cwd_state);
      assert (VALID_STATUS (s));
      UPDATE_STATUS (status, s);
    }

  if (x->require_restore_cwd && cwd_state && cwd_state->saved_errno != 0)
    {
      error (0, cwd_state->saved_errno,
	     _("cannot restore current working directory"));
      status = RM_ERROR;
    }

  ds_free (ds);

  if (cwd_state && cwd_state->saved_errno == 0)
    free_cwd (&cwd_state->saved_cwd);

  free (cwd_state);

  return status;
}
