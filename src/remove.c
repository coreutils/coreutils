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

#include "system.h"
#include "cycle-check.h"
#include "dirfd.h"
#include "dirname.h"
#include "error.h"
#include "euidaccess.h"
#include "euidaccess-stat.h"
#include "file-type.h"
#include "hash.h"
#include "hash-pjw.h"
#include "lstat.h"
#include "obstack.h"
#include "openat.h"
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

/* Define to the options that make open (or openat) fail to open
   a symlink.  Define to 0 if there are no such options.
   This is useful because it permits us to skip the `fstatat (".",...'
   and dev/ino comparison in AD_push.  */
#if defined O_NOFOLLOW
# define OPEN_NO_FOLLOW_SYMLINK O_NOFOLLOW
#else
# define OPEN_NO_FOLLOW_SYMLINK 0
#endif

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

enum
  {
    OPENAT_CWD_RESTORE__REQUIRE = false,
    OPENAT_CWD_RESTORE__ALLOW_FAILURE = true
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
enum { HT_UNREMOVABLE_INITIAL_CAPACITY = 13 };

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

  /* The directory's dev/ino.  Used to ensure that a malicious user does
     not replace a directory we're about to process with a symlink to
     some other directory.  */
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

  assert (n_lengths > 0);
  size_t top_len = length[n_lengths - 1];
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
  assert (0 < AD_stack_height (ds));

  /* operate on Active_dir.  pop and free top entry */
  struct AD_ent *top = AD_stack_top (ds);
  if (top->unremovable)
    hash_free (top->unremovable);
  obstack_blank (&ds->Active_dir, -(int) sizeof (struct AD_ent));
}

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
ds_clear (Dirstack_state *ds)
{
  obstack_free (&ds->dir_stack, obstack_finish (&ds->dir_stack));
  obstack_free (&ds->len_stack, obstack_finish (&ds->len_stack));
  while (0 < AD_stack_height (ds))
    AD_stack_pop (ds);
  obstack_free (&ds->Active_dir, obstack_finish (&ds->Active_dir));
}

static void
ds_free (Dirstack_state *ds)
{
  obstack_free (&ds->dir_stack, NULL);
  obstack_free (&ds->len_stack, NULL);
  obstack_free (&ds->Active_dir, NULL);
  free (ds);
}

/* Pop the active directory (AD) stack and move *DIRP `up' one level,
   safely.  Moving `up' usually means opening `..', but when we've just
   finished recursively processing a command-line directory argument,
   there's nothing left on the stack, so set *DIRP to NULL in that case.
   The idea is to return with *DIRP opened on the parent directory,
   assuming there are entries in that directory that we need to remove.

   Whenever using chdir '..' (virtually, now, via openat), verify
   that the post-chdir dev/ino numbers for `.' match the saved ones.
   If any system call fails or if dev/ino don't match then give a
   diagnostic and longjump out.
   Set *PREV_DIR to the name (in malloc'd storage) of the
   directory (usually now empty) from which we're coming, and which
   corresponds to the input value of *DIRP.  */
static void
AD_pop_and_chdir (DIR **dirp, Dirstack_state *ds, char **prev_dir)
{
  enum RM_status old_status = AD_stack_top(ds)->status;
  struct AD_ent *top;

  /* Get the name of the current (but soon to be `previous') directory
     from the top of the stack.  */
  *prev_dir = top_dir (ds);

  AD_stack_pop (ds);
  pop_dir (ds);
  top = AD_stack_top (ds);

  /* Propagate any failure to parent.  */
  UPDATE_STATUS (top->status, old_status);

  assert (AD_stack_height (ds));

  if (1 < AD_stack_height (ds))
    {
      struct stat sb;
      int fd = openat (dirfd (*dirp), "..", O_RDONLY);
      if (CLOSEDIR (*dirp) != 0)
	{
	  error (0, errno, _("FATAL: failed to close directory %s"),
		 quote (full_filename (*prev_dir)));
	  longjmp (ds->current_arg_jumpbuf, 1);
	}

      /* The above fails with EACCES when *DIRP is readable but not
	 searchable, when using Solaris' openat.  Without this openat
	 call, tests/rm2 would fail to remove directories a/2 and a/3.  */
      if (fd < 0)
	fd = openat (AT_FDCWD, full_filename ("."), O_RDONLY);

      if (fd < 0)
	{
	  error (0, errno, _("FATAL: cannot open .. from %s"),
		 quote (full_filename (*prev_dir)));
	  longjmp (ds->current_arg_jumpbuf, 1);
	}

      if (fstat (fd, &sb))
	{
	  error (0, errno,
		 _("FATAL: cannot ensure %s (returned to via ..) is safe"),
		 quote (full_filename (".")));
	  close (fd);
	  longjmp (ds->current_arg_jumpbuf, 1);
	}

      /*  Ensure that post-chdir dev/ino match the stored ones.  */
      if ( ! SAME_INODE (sb, top->dev_ino))
	{
	  error (0, 0, _("FATAL: directory %s changed dev/ino"),
		 quote (full_filename (".")));
	  close (fd);
	  longjmp (ds->current_arg_jumpbuf, 1);
	}

      *dirp = fdopendir (fd);
      if (*dirp == NULL)
	{
	  error (0, errno, _("FATAL: cannot return to .. from %s"),
		 quote (full_filename (".")));
	  close (fd);
	  longjmp (ds->current_arg_jumpbuf, 1);
	}
    }
  else
    {
      if (CLOSEDIR (*dirp) != 0)
	{
	  error (0, errno, _("FATAL: failed to close directory %s"),
		 quote (full_filename (*prev_dir)));
	  longjmp (ds->current_arg_jumpbuf, 1);
	}
      *dirp = NULL;
    }
}

/* Initialize *HT if it is NULL.
   Insert FILENAME into HT.  */
static void
AD_mark_helper (Hash_table **ht, char const *filename)
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
  char const *curr = top_dir (ds);

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
   is the result of calling lstat on DIR from the parent of DIR.
   Return true upon success, false if fstatat "." fails, and longjump
   (skipping the entire command line argument we're dealing with) if
   someone has replaced DIR with e.g., a symlink to some other directory.  */
static void
AD_push (int fd_cwd, Dirstack_state *ds, char const *dir,
	 struct stat const *dir_sb_from_parent)
{
  struct AD_ent *top;

  push_dir (ds, dir);

  /* If our uses of openat are guaranteed not to
     follow a symlink, then we can skip this check.  */
  if ( ! OPEN_NO_FOLLOW_SYMLINK)
    {
      struct stat sb;
      if (fstat (fd_cwd, &sb) != 0)
	{
	  error (0, errno, _("FATAL: cannot enter directory %s"),
		 quote (full_filename (".")));
	  longjmp (ds->current_arg_jumpbuf, 1);
	}

      if ( ! SAME_INODE (sb, *dir_sb_from_parent))
	{
	  error (0, 0,
		 _("FATAL: just-changed-to directory %s changed dev/ino"),
		 quote (full_filename (".")));
	  longjmp (ds->current_arg_jumpbuf, 1);
	}
    }

  /* Extend the stack.  */
  obstack_blank (&ds->Active_dir, sizeof (struct AD_ent));

  {
    size_t n_lengths = obstack_object_size (&ds->len_stack) / sizeof (size_t);
    if (AD_stack_height (ds) != n_lengths + 1)
      error (0, 0, "%lu %lu", (unsigned long) AD_stack_height (ds),
	     (unsigned long) n_lengths);
    assert (AD_stack_height (ds) == n_lengths + 1);
  }

  /* Fill in the new values.  */
  top = AD_stack_top (ds);
  top->dev_ino.st_dev = dir_sb_from_parent->st_dev;
  top->dev_ino.st_ino = dir_sb_from_parent->st_ino;
  top->unremovable = NULL;
}

static bool
AD_is_removable (Dirstack_state const *ds, char const *file)
{
  struct AD_ent *top = AD_stack_top (ds);
  return ! (top->unremovable && hash_lookup (top->unremovable, file));
}

/* Return true if DIR is determined to be an empty directory
   or if fdopendir or readdir fails.  */
static bool
is_empty_dir (int fd_cwd, char const *dir)
{
  DIR *dirp;
  struct dirent const *dp;
  int saved_errno;
  int fd = openat (fd_cwd, dir, O_RDONLY);

  if (fd < 0)
    return false;

  dirp = fdopendir (fd);
  if (dirp == NULL)
    {
      close (fd);
      return false;
    }

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
   If lstat (aka fstatat) succeeds, set *BUF_P to BUF.
   This is to avoid calling euidaccess when FILE is a symlink.  */
static bool
write_protected_non_symlink (int fd_cwd,
			     char const *file,
			     Dirstack_state const *ds,
			     struct stat **buf_p,
			     struct stat *buf)
{
  if (fstatat (fd_cwd, file, buf, AT_SYMLINK_NOFOLLOW) != 0)
    return false;
  *buf_p = buf;
  if (S_ISLNK (buf->st_mode))
    return false;
  /* Here, we know FILE is not a symbolic link.  */

  /* In order to be reentrant -- i.e., to avoid changing the working
     directory, and at the same time to be able to deal with alternate
     access control mechanisms (ACLs, xattr-style attributes) and
     arbitrarily deep trees -- we need a function like eaccessat, i.e.,
     like Solaris' eaccess, but fd-relative, in the spirit of openat.  */

  /* In the absence of a native eaccessat function, here are some of
     the implementation choices [#4 and #5 were suggested by Paul Eggert]:
     1) call openat with O_WRONLY|O_NOCTTY
	Disadvantage: may create the file and doesn't work for directory,
	may mistakenly report `unwritable' for EROFS or ACLs even though
	perm bits say the file is writable.

     2) fake eaccessat (save_cwd, fchdir, call euidaccess, restore_cwd)
	Disadvantage: changes working directory (not reentrant) and can't
	work if save_cwd fails.

     3) if (euidaccess (full_filename (file), W_OK) == 0)
	Disadvantage: doesn't work if full_filename is too long.
	Inefficient for very deep trees (O(Depth^2)).

     4) If the full pathname is sufficiently short (say, less than
	PATH_MAX or 8192 bytes, whichever is shorter):
	use method (3) (i.e., euidaccess (full_filename (file), W_OK));
	Otherwise: vfork, fchdir in the child, run euidaccess in the
	child, then the child exits with a status that tells the parent
	whether euidaccess succeeded.

	This avoids the O(N**2) algorithm of method (3), and it also avoids
	the failure-due-to-too-long-file-names of method (3), but it's fast
	in the normal shallow case.  It also avoids the lack-of-reentrancy
	and the save_cwd problems.
	Disadvantage; it uses a process slot for very-long file names,
	and would be very slow for hierarchies with many such files.

     5) If the full file name is sufficiently short (say, less than
	PATH_MAX or 8192 bytes, whichever is shorter):
	use method (3) (i.e., euidaccess (full_filename (file), W_OK));
	Otherwise: look just at the file bits.  Perhaps issue a warning
	the first time this occurs.

	This is like (4), except for the "Otherwise" case where it isn't as
	"perfect" as (4) but is considerably faster.  It conforms to current
	POSIX, and is uniformly better than what Solaris and FreeBSD do (they
	mess up with long file names). */

  {
    /* This implements #5: */
    size_t file_name_len
      = obstack_object_size (&ds->dir_stack) + strlen (file);

    return (file_name_len < MIN (PATH_MAX, 8192)
	    ? euidaccess (full_filename (file), W_OK) != 0 && errno == EACCES
	    : euidaccess_stat (buf, W_OK) != 0);
  }
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
prompt (int fd_cwd, Dirstack_state const *ds, char const *filename,
	struct rm_options const *x, enum Prompt_action mode,
	Ternary *is_dir, Ternary *is_empty)
{
  bool write_protected = false;
  struct stat *sbuf = NULL;
  struct stat buf;

  *is_empty = T_UNKNOWN;
  *is_dir = T_UNKNOWN;

  if (((!x->ignore_missing_files & (x->interactive | x->stdin_tty))
       && (write_protected = write_protected_non_symlink (fd_cwd, filename,
							  ds, &sbuf, &buf)))
      || x->interactive)
    {
      if (sbuf == NULL)
	{
	  sbuf = &buf;
	  if (fstatat (fd_cwd, filename, sbuf, AT_SYMLINK_NOFOLLOW))
	    {
	      /* lstat failed.  This happens e.g., with `rm '''.  */
	      error (0, errno, _("cannot remove %s"),
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
	    && ((*is_empty = (is_empty_dir (fd_cwd, filename) ? T_YES : T_NO))
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

/* Return true if FILENAME is a directory (and not a symlink to a directory).
   Otherwise, including the case in which lstat fails, return false.
   Do not modify errno.  */
static inline bool
is_dir_lstat (char const *filename)
{
  struct stat sbuf;
  int saved_errno = errno;
  bool is_dir = lstat (filename, &sbuf) == 0 && S_ISDIR (sbuf.st_mode);
  errno = saved_errno;
  return is_dir;
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

#define DO_UNLINK(Fd_cwd, Filename, X)					\
  do									\
    {									\
      if (unlinkat (Fd_cwd, Filename, 0) == 0)				\
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

#define DO_RMDIR(Fd_cwd, Filename, X)			\
  do							\
    {							\
      if (unlinkat (Fd_cwd, Filename, AT_REMOVEDIR) == 0) /* rmdir */ \
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
remove_entry (int fd_cwd, Dirstack_state const *ds, char const *filename,
	      struct rm_options const *x, struct dirent const *dp)
{
  Ternary is_dir;
  Ternary is_empty_directory;
  enum RM_status s = prompt (fd_cwd, ds, filename, x, PA_DESCEND_INTO_DIR,
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
     go to unlink it, but in the mean time, a malicious someone could have
     replaced it with a directory.  */

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
	DO_RMDIR (fd_cwd, filename, x);

      /* If we happen to know that FILENAME is a directory, return now
	 and let the caller remove it -- this saves the overhead of a failed
	 unlink call.  If FILENAME is a command-line argument, then dp is NULL,
	 so we'll first try to unlink it.  Using unlink here is ok, because it
	 cannot remove a directory.  */
      if ((dp && DT_MUST_BE (dp, DT_DIR)) || is_dir == T_YES)
	return RM_NONEMPTY_DIR;

      DO_UNLINK (fd_cwd, filename, x);

      /* Upon a failed attempt to unlink a directory, most non-Linux systems
	 set errno to the POSIX-required value EPERM.  In that case, change
	 errno to EISDIR so that we emit a better diagnostic.  */
      if (! x->recursive && errno == EPERM && is_dir_lstat (filename))
	errno = EISDIR;

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
	      if (fstatat (fd_cwd, filename, &sbuf, AT_SYMLINK_NOFOLLOW))
		{
		  if (errno == ENOENT && x->ignore_missing_files)
		    return RM_OK;

		  error (0, errno, _("cannot remove %s"),
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
	  DO_UNLINK (fd_cwd, filename, x);

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
	  DO_RMDIR (fd_cwd, filename, x);
	  /* Don't diagnose any failure here.
	     It'll be detected when the caller tries another way.  */
	}
    }

  return RM_NONEMPTY_DIR;
}

/* Given FD_CWD, the file descriptor for an open directory,
   open its subdirectory F (F is already `known' to be a directory,
   so if it is no longer one, someone is playing games), return a DIR*
   pointer for F, and put F's `stat' data in *SUBDIR_SB.
   Upon failure give a diagnostic and return NULL.
   If PREV_ERRNO is nonzero, it is the errno value from a preceding failed
   unlink- or rmdir-like system call -- use that value instead of ENOTDIR
   if an opened file turns out not to be a directory.  This is important
   when the preceding non-dir-unlink failed due to e.g., EPERM or EACCES.
   The caller must set OPENAT_CWD_RESTORE_ALLOW_FAILURE to true the first
   time this function is called for each command-line-specified directory.
   Set *CWD_RESTORE_FAILED if OPENAT_CWD_RESTORE_ALLOW_FAILURE is true
   and openat_ro fails to restore the initial working directory.
   CWD_RESTORE_FAILED may be NULL.  */
static DIR *
fd_to_subdirp (int fd_cwd, char const *f,
	       struct rm_options const *x, int prev_errno,
	       struct stat *subdir_sb, Dirstack_state *ds,
	       bool openat_cwd_restore_allow_failure,
	       bool *cwd_restore_failed)
{
  int fd_sub;
  bool dummy;

  /* Record dev/ino of F.  We may compare them against saved values
     to thwart any attempt to subvert the traversal.  They are also used
     to detect directory cycles.  */
  if ((fd_sub = (openat_cwd_restore_allow_failure
		 ? openat_ro (fd_cwd, f, O_RDONLY | OPEN_NO_FOLLOW_SYMLINK,
			      (cwd_restore_failed
			       ? cwd_restore_failed : &dummy))
		 : openat (fd_cwd, f, O_RDONLY | OPEN_NO_FOLLOW_SYMLINK))) < 0
      || fstat (fd_sub, subdir_sb) != 0)
    {
      if (errno != ENOENT || !x->ignore_missing_files)
	error (0, errno,
	       _("cannot remove %s"), quote (full_filename (f)));
      if (0 <= fd_sub)
	close (fd_sub);
      return NULL;
    }

  if (! S_ISDIR (subdir_sb->st_mode))
    {
      error (0, prev_errno ? prev_errno : ENOTDIR, _("cannot remove %s"),
	     quote (full_filename (f)));
      close (fd_sub);
      return NULL;
    }

  DIR *subdir_dirp = fdopendir (fd_sub);
  if (subdir_dirp == NULL)
    {
      if (errno != ENOENT || !x->ignore_missing_files)
	error (0, errno, _("cannot open directory %s"),
	       quote (full_filename (f)));
      close (fd_sub);
      return NULL;
    }

  return subdir_dirp;
}

/* Remove entries in the directory open on DIRP
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
remove_cwd_entries (DIR **dirp,
		    Dirstack_state *ds, char **subdir, struct stat *subdir_sb,
		    struct rm_options const *x)
{
  struct AD_ent *top = AD_stack_top (ds);
  enum RM_status status = top->status;
  size_t n_unlinked_since_opendir_or_last_rewind = 0;

  assert (VALID_STATUS (status));
  *subdir = NULL;

  while (1)
    {
      struct dirent const *dp;
      enum RM_status tmp_status;
      const char *f;

      /* Set errno to zero so we can distinguish between a readdir failure
	 and when readdir simply finds that there are no more entries.  */
      errno = 0;
      dp = readdir_ignoring_dot_and_dotdot (*dirp);
      if (dp == NULL)
	{
	  if (errno)
	    {
	      /* fall through */
	    }
	  else if (CONSECUTIVE_READDIR_UNLINK_THRESHOLD
		   < n_unlinked_since_opendir_or_last_rewind)
	    {
	      /* Call rewinddir if we've called unlink or rmdir so many times
		 (since the opendir or the previous rewinddir) that this
		 NULL-return may be the symptom of a buggy readdir.  */
	      rewinddir (*dirp);
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
      tmp_status = remove_entry (dirfd (*dirp), ds, f, x, dp);
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
	    DIR *subdir_dirp = fd_to_subdirp (dirfd (*dirp), f,
					      x, errno, subdir_sb, ds,
					      OPENAT_CWD_RESTORE__REQUIRE,
					      NULL);
	    if (subdir_dirp == NULL)
	      {
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
	    if (CLOSEDIR (*dirp) != 0)
	      {
		error (0, 0, _("failed to close directory %s"),
		       quote (full_filename (".")));
		status = RM_ERROR;
	      }
	    *dirp = subdir_dirp;

	    break;
	  }
	}

      /* Record status for this directory.  */
      UPDATE_STATUS (top->status, status);

      if (*subdir)
	break;
    }

  /* Ensure that *dirp is not NULL and that its file descriptor is valid.  */
  assert (*dirp != NULL);
  assert (0 <= fcntl (dirfd (*dirp), F_GETFD));

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
    Return an RM_status value to indicate success or failure.  */

static enum RM_status
remove_dir (int fd_cwd, Dirstack_state *ds, char const *dir,
	    struct rm_options const *x, bool *cwd_restore_failed)
{
  enum RM_status status;
  struct stat dir_sb;

  /* There is a race condition in that an attacker could replace the nonempty
     directory, DIR, with a symlink between the preceding call to rmdir
     (unlinkat, in our caller) and fd_to_subdirp's openat call.  But on most
     systems, even those without openat, this isn't a problem, since we ensure
     that opening a symlink will fail, when that is possible.  Otherwise,
     fd_to_subdirp's fstat, along with the `fstatat (".",...' and the dev/ino
     comparison in AD_push ensure that we detect it and fail.  */

  DIR *dirp = fd_to_subdirp (fd_cwd, dir, x, 0, &dir_sb, ds,
			     OPENAT_CWD_RESTORE__ALLOW_FAILURE,
			     cwd_restore_failed);

  if (dirp == NULL)
    return RM_ERROR;

  if (ROOT_DEV_INO_CHECK (x->root_dev_ino, &dir_sb))
    {
      ROOT_DEV_INO_WARN (full_filename (dir));
      status = RM_ERROR;
      goto closedir_and_return;
    }

  AD_push (dirfd (dirp), ds, dir, &dir_sb);
  AD_INIT_OTHER_MEMBERS ();

  status = RM_OK;

  while (1)
    {
      char *subdir = NULL;
      struct stat subdir_sb;
      enum RM_status tmp_status;

      tmp_status = remove_cwd_entries (&dirp, ds, &subdir, &subdir_sb, x);

      if (tmp_status != RM_OK)
	{
	  UPDATE_STATUS (status, tmp_status);
	  AD_mark_current_as_unremovable (ds);
	}
      if (subdir)
	{
	  AD_push (dirfd (dirp), ds, subdir, &subdir_sb);
	  AD_INIT_OTHER_MEMBERS ();

	  free (subdir);
	  continue;
	}

      /* Execution reaches this point when we've removed the last
	 removable entry from the current directory.  */
      {
	/* The name of the directory that we have just processed,
	   nominally removing all of its contents.  */
	char *empty_dir;

	AD_pop_and_chdir (&dirp, ds, &empty_dir);
	int fd = (dirp != NULL ? dirfd (dirp) : AT_FDCWD);
	assert (dirp != NULL || AD_stack_height (ds) == 1);

	/* Try to remove EMPTY_DIR only if remove_cwd_entries succeeded.  */
	if (tmp_status == RM_OK)
	  {
	    /* This does a little more work than necessary when it actually
	       prompts the user.  E.g., we already know that D is a directory
	       and that it's almost certainly empty, yet we lstat it.
	       But that's no big deal since we're interactive.  */
	    Ternary is_dir;
	    Ternary is_empty;
	    enum RM_status s = prompt (fd, ds, empty_dir, x,
				       PA_REMOVE_DIR, &is_dir, &is_empty);

	    if (s != RM_OK)
	      {
		free (empty_dir);
		status = s;
		goto closedir_and_return;
	      }

	    if (unlinkat (fd, empty_dir, AT_REMOVEDIR) == 0)
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

  /* If the first/final hash table of unremovable entries was used,
     free it here.  */
  AD_stack_pop (ds);

 closedir_and_return:;
  if (dirp != NULL && CLOSEDIR (dirp) != 0)
    {
      error (0, 0, _("failed to close directory %s"),
	     quote (full_filename (".")));
      status = RM_ERROR;
    }

  return status;
}

/* Remove the file or directory specified by FILENAME.
   Return RM_OK if it is removed, and RM_ERROR or RM_USER_DECLINED if not.  */

static enum RM_status
rm_1 (Dirstack_state *ds, char const *filename,
      struct rm_options const *x, bool *cwd_restore_failed)
{
  char const *base = base_name (filename);
  if (DOT_OR_DOTDOT (base))
    {
      error (0, 0, _("cannot remove `.' or `..'"));
      return RM_ERROR;
    }

  AD_push_initial (ds);
  AD_INIT_OTHER_MEMBERS ();

  /* Put `status' in static storage, so it can't be clobbered
     by the potential longjmp into this function.  */
  static enum RM_status status;
  int fd_cwd = AT_FDCWD;
  status = remove_entry (fd_cwd, ds, filename, x, NULL);
  if (status == RM_NONEMPTY_DIR)
    {
      /* In the event that remove_dir->remove_cwd_entries detects
	 a directory cycle, arrange to fail, give up on this FILE, but
	 continue on with any other arguments.  */
      if (setjmp (ds->current_arg_jumpbuf))
	status = RM_ERROR;
      else
	{
	  bool t_cwd_restore_failed = false;
	  status = remove_dir (fd_cwd, ds, filename, x, &t_cwd_restore_failed);
	  *cwd_restore_failed |= t_cwd_restore_failed;
	}
    }

  ds_clear (ds);

  return status;
}

/* Remove all files and/or directories specified by N_FILES and FILE.
   Apply the options in X.  */
extern enum RM_status
rm (size_t n_files, char const *const *file, struct rm_options const *x)
{
  enum RM_status status = RM_OK;
  Dirstack_state *ds = ds_init ();
  bool cwd_restore_failed = false;

  for (size_t i = 0; i < n_files; i++)
    {
      if (cwd_restore_failed && IS_RELATIVE_FILE_NAME (file[i]))
	{
	  error (0, 0, _("cannot remove relative-named %s"), quote (file[i]));
	  status = RM_ERROR;
	  continue;
	}

      cycle_check_init (&ds->cycle_check_state);
      enum RM_status s = rm_1 (ds, file[i], x, &cwd_restore_failed);
      assert (VALID_STATUS (s));
      UPDATE_STATUS (status, s);
    }

  if (x->require_restore_cwd && cwd_restore_failed)
    {
      error (0, 0,
	     _("cannot restore current working directory"));
      status = RM_ERROR;
    }

  ds_free (ds);

  return status;
}
