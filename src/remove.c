/* remove.c -- core functions for removing files and directories
   Copyright (C) 88, 90, 91, 1994-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Extracted from rm.c and librarified, then rewritten by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <setjmp.h>
#include <assert.h>

#include "system.h"
#include "cycle-check.h"
#include "error.h"
#include "euidaccess-stat.h"
#include "file-type.h"
#include "hash.h"
#include "hash-pjw.h"
#include "obstack.h"
#include "quote.h"
#include "remove.h"
#include "root-dev-ino.h"
#include "unlinkdir.h"
#include "write-any-file.h"
#include "yesno.h"

/* Avoid shadowing warnings because these are functions declared
   in dirname.h as well as locals used below.  */
#define dir_name rm_dir_name
#define dir_len rm_dir_len

/* This is the maximum number of consecutive readdir/unlink calls that
   can be made (with no intervening rewinddir or closedir/opendir) before
   triggering a bug that makes readdir return NULL even though some
   directory entries have not been processed.  The bug afflicts SunOS's
   readdir when applied to ufs file systems and Darwin 6.5's (and OSX
   v.10.3.8's) HFS+.  This maximum is conservative in that demonstrating
   the problem requires a directory containing at least 16 deletable
   entries (which doesn't count . and ..).
   This problem also affects Darwin 7.9.0 (aka MacOS X 10.3.9) on HFS+
   and NFS-mounted file systems, but not vfat ones.  */
enum
  {
    CONSECUTIVE_READDIR_UNLINK_THRESHOLD = 10
  };

/* If the heuristics in preprocess_dir suggest that there
   are fewer than this many entries in a directory, then it
   skips the preprocessing altogether.  */
enum
{
  INODE_SORT_DIR_ENTRIES_THRESHOLD = 10000
};

/* FIXME: in 2009, or whenever Darwin 7.9.0 (aka MacOS X 10.3.9) is no
   longer relevant, remove this work-around code.  Then, there will be
   no need to perform the extra rewinddir call, ever.  */
#define NEED_REWIND(readdir_unlink_count) \
  (CONSECUTIVE_READDIR_UNLINK_THRESHOLD <= (readdir_unlink_count))

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

/* D_TYPE(D) is the type of directory entry D if known, DT_UNKNOWN
   otherwise.  */
#if HAVE_STRUCT_DIRENT_D_TYPE
# define D_TYPE(d) ((d)->d_type)
#else
# define D_TYPE(d) DT_UNKNOWN

/* Any int values will do here, so long as they're distinct.
   Undef any existing macros out of the way.  */
# undef DT_UNKNOWN
# undef DT_DIR
# undef DT_LNK
# define DT_UNKNOWN 0
# define DT_DIR 1
# define DT_LNK 2
#endif

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

/* A static buffer and its allocated size, these variables are used by
   xfull_filename and full_filename to form full, relative file names.  */
static char *g_buf;
static size_t g_n_allocated;

/* Like fstatat, but cache the result.  If ST->st_size is -1, the
   status has not been gotten yet.  If less than -1, fstatat failed
   with errno == ST->st_ino.  Otherwise, the status has already
   been gotten, so return 0.  */
static int
cache_fstatat (int fd, char const *file, struct stat *st, int flag)
{
  if (st->st_size == -1 && fstatat (fd, file, st, flag) != 0)
    {
      st->st_size = -2;
      st->st_ino = errno;
    }
  if (0 <= st->st_size)
    return 0;
  errno = (int) st->st_ino;
  return -1;
}

/* Initialize a fstatat cache *ST.  Return ST for convenience.  */
static inline struct stat *
cache_stat_init (struct stat *st)
{
  st->st_size = -1;
  return st;
}

/* Return true if *ST has been statted.  */
static inline bool
cache_statted (struct stat *st)
{
  return (st->st_size != -1);
}

/* Return true if *ST has been statted successfully.  */
static inline bool
cache_stat_ok (struct stat *st)
{
  return (0 <= st->st_size);
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

/* Obstack allocator: longjump on failure.  */
static void *
rm_malloc (void *v_jumpbuf, long size)
{
  jmp_buf *jumpbuf = v_jumpbuf;
  void *p = malloc (size);
  if (p)
    return p;
  longjmp (*jumpbuf, 1);
}

/* With the 2-arg allocator, we must also provide a two-argument freer.  */
static void
rm_free (void *v_jumpbuf ATTRIBUTE_UNUSED, void *ptr)
{
  free (ptr);
}

static inline void
push_dir (Dirstack_state *ds, const char *dir_name)
{
  size_t len = strlen (dir_name);

  /* Don't copy trailing slashes.  */
  while (1 < len && dir_name[len - 1] == '/')
    --len;

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
top_dir (Dirstack_state *ds)
{
  size_t n_lengths = obstack_object_size (&ds->len_stack) / sizeof (size_t);
  size_t *length = obstack_base (&ds->len_stack);
  size_t top_len = length[n_lengths - 1];
  char const *p = obstack_next_free (&ds->dir_stack) - top_len;
  char *q = malloc (top_len);
  if (q == NULL)
    longjmp (ds->current_arg_jumpbuf, 1);
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

/* Using the global directory name obstack, create the full name of FILENAME.
   Return it in sometimes-realloc'd space that should not be freed by the
   caller.  Realloc as necessary.  If realloc fails, return NULL.  */

static char *
full_filename0 (Dirstack_state const *ds, const char *filename)
{
  size_t dir_len = obstack_object_size (&ds->dir_stack);
  char *dir_name = obstack_base (&ds->dir_stack);
  size_t filename_len = strlen (filename);
  size_t n_bytes_needed = dir_len + filename_len + 1;

  if (g_n_allocated < n_bytes_needed)
    {
      char *new_buf = realloc (g_buf, n_bytes_needed);
      if (new_buf == NULL)
        return NULL;

      g_buf = new_buf;
      g_n_allocated = n_bytes_needed;
    }

  if (STREQ (filename, ".") && dir_len)
    {
      /* FILENAME is just `.' and dir_len is nonzero.
         Copy the directory part, omitting the trailing slash,
         and append a trailing zero byte.  */
      char *p = mempcpy (g_buf, dir_name, dir_len - 1);
      *p = 0;
    }
  else
    {
      /* Copy the directory part, including trailing slash, and then
         append the filename part, including a trailing zero byte.  */
      memcpy (mempcpy (g_buf, dir_name, dir_len), filename, filename_len + 1);
      assert (strlen (g_buf) + 1 == n_bytes_needed);
    }

  return g_buf;
}

/* Using the global directory name obstack, create the full name of FILENAME.
   Return it in sometimes-realloc'd space that should not be freed by the
   caller.  Realloc as necessary.  If realloc fails, die.  */

static char *
xfull_filename (Dirstack_state const *ds, const char *filename)
{
  char *buf = full_filename0 (ds, filename);
  if (buf == NULL)
    xalloc_die ();
  return buf;
}

/* Using the global directory name obstack, create the full name FILENAME.
   Return it in sometimes-realloc'd space that should not be freed by the
   caller.  Realloc as necessary.  If realloc fails, use a static buffer
   and put as long a suffix in that buffer as possible.  Be careful not
   to change errno.  */

#define full_filename(Filename) full_filename_ (ds, Filename)
static char *
full_filename_ (Dirstack_state const *ds, const char *filename)
{
  int saved_errno = errno;
  char *full_name = full_filename0 (ds, filename);
  if (full_name)
    {
      errno = saved_errno;
      return full_name;
    }

  {
#define SBUF_SIZE 512
#define ELLIPSES_PREFIX "[...]"
    static char static_buf[SBUF_SIZE];
    bool file_truncated;
    bool dir_truncated;
    size_t n_bytes_remaining;
    char *p;
    char *dir_name = obstack_base (&ds->dir_stack);
    size_t dir_len = obstack_object_size (&ds->dir_stack);

    free (g_buf);
    n_bytes_remaining = right_justify (static_buf, SBUF_SIZE, filename,
                                       strlen (filename) + 1, &p,
                                       &file_truncated);
    right_justify (static_buf, n_bytes_remaining, dir_name, dir_len,
                   &p, &dir_truncated);
    if (file_truncated || dir_truncated)
      {
        memcpy (static_buf, ELLIPSES_PREFIX,
                sizeof (ELLIPSES_PREFIX) - 1);
      }
    errno = saved_errno;
    return p;
  }
}

static inline size_t
AD_stack_height (Dirstack_state const *ds)
{
  return obstack_object_size (&ds->Active_dir) / sizeof (struct AD_ent);
}

static inline struct AD_ent *
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

static void
AD_stack_clear (Dirstack_state *ds)
{
  while (0 < AD_stack_height (ds))
    {
      AD_stack_pop (ds);
    }
}

/* Initialize obstack O just enough so that it may be freed
   with obstack_free.  */
static void
obstack_init_minimal (struct obstack *o)
{
  o->chunk = NULL;
}

static void
ds_init (Dirstack_state *ds)
{
  unsigned int i;
  struct obstack *o[3];
  o[0] = &ds->dir_stack;
  o[1] = &ds->len_stack;
  o[2] = &ds->Active_dir;

  /* Ensure each of these is NULL, in case init/allocation
     fails and we end up calling ds_free on all three while only
     one or two has been initialized.  */
  for (i = 0; i < 3; i++)
    obstack_init_minimal (o[i]);

  for (i = 0; i < 3; i++)
    obstack_specify_allocation_with_arg
      (o[i], 0, 0, rm_malloc, rm_free, &ds->current_arg_jumpbuf);
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
}

/* Pop the active directory (AD) stack and prepare to move `up' one level,
   safely.  Moving `up' usually means opening `..', but when we've just
   finished recursively processing a command-line directory argument,
   there's nothing left on the stack, so set *FDP to AT_FDCWD in that case.
   The idea is to return with *FDP opened on the parent directory,
   assuming there are entries in that directory that we need to remove.

   Note that we must not call opendir (or fdopendir) just yet, since
   the caller must first remove the directory we're coming from.
   That is because some file system implementations cache readdir
   results at opendir time; so calling opendir, rmdir, readdir would
   return an entry for the just-removed directory.

   Whenever using chdir '..' (virtually, now, via openat), verify
   that the post-chdir dev/ino numbers for `.' match the saved ones.
   If any system call fails or if dev/ino don't match, then give a
   diagnostic and longjump out.
   Return the name (in malloc'd storage) of the
   directory (usually now empty) from which we're coming, and which
   corresponds to the input value of DIRP.

   Finally, note that while this function's name is no longer as
   accurate as it once was (it no longer calls chdir), it does open
   the destination directory.  */
static char *
AD_pop_and_chdir (DIR *dirp, int *fdp, Dirstack_state *ds)
{
  struct AD_ent *leaf_dir_ent = AD_stack_top(ds);
  struct dev_ino leaf_dev_ino = leaf_dir_ent->dev_ino;
  enum RM_status old_status = leaf_dir_ent->status;
  struct AD_ent *top;

  /* Get the name of the current (but soon to be `previous') directory
     from the top of the stack.  */
  char *prev_dir = top_dir (ds);

  AD_stack_pop (ds);
  pop_dir (ds);
  top = AD_stack_top (ds);

  /* If the directory we're about to leave (and try to rmdir)
     is the one whose dev_ino is being used to detect a cycle,
     reset cycle_check_state.dev_ino to that of the parent.
     Otherwise, once that directory is removed, its dev_ino
     could be reused in the creation (by some other process)
     of a directory that this rm process would encounter,
     which would result in a false-positive cycle indication.  */
  CYCLE_CHECK_REFLECT_CHDIR_UP (&ds->cycle_check_state,
                                top->dev_ino, leaf_dev_ino);

  /* Propagate any failure to parent.  */
  UPDATE_STATUS (top->status, old_status);

  assert (AD_stack_height (ds));

  if (1 < AD_stack_height (ds))
    {
      struct stat sb;
      int fd = openat (dirfd (dirp), "..", O_RDONLY);
      if (closedir (dirp) != 0)
        {
          error (0, errno, _("FATAL: failed to close directory %s"),
                 quote (full_filename (prev_dir)));
          goto next_cmdline_arg;
        }

      /* The above fails with EACCES when DIRP is readable but not
         searchable, when using Solaris' openat.  Without this openat
         call, tests/rm2 would fail to remove directories a/2 and a/3.  */
      if (fd < 0)
        fd = openat (AT_FDCWD, xfull_filename (ds, "."), O_RDONLY);

      if (fd < 0)
        {
          error (0, errno, _("FATAL: cannot open .. from %s"),
                 quote (full_filename (prev_dir)));
          goto next_cmdline_arg;
        }

      if (fstat (fd, &sb))
        {
          error (0, errno,
                 _("FATAL: cannot ensure %s (returned to via ..) is safe"),
                 quote (full_filename (".")));
          goto close_and_next;
        }

      /*  Ensure that post-chdir dev/ino match the stored ones.  */
      if ( ! SAME_INODE (sb, top->dev_ino))
        {
          error (0, 0, _("FATAL: directory %s changed dev/ino"),
                 quote (full_filename (".")));
        close_and_next:;
          close (fd);

        next_cmdline_arg:;
          free (prev_dir);
          longjmp (ds->current_arg_jumpbuf, 1);
        }
      *fdp = fd;
    }
  else
    {
      if (closedir (dirp) != 0)
        {
          error (0, errno, _("FATAL: failed to close directory %s"),
                 quote (full_filename (prev_dir)));
          goto next_cmdline_arg;
        }
      *fdp = AT_FDCWD;
    }

  return prev_dir;
}

/* Initialize *HT if it is NULL.  Return *HT.  */
static Hash_table *
AD_ensure_initialized (Hash_table **ht)
{
  if (*ht == NULL)
    {
      *ht = hash_initialize (HT_UNREMOVABLE_INITIAL_CAPACITY, NULL, hash_pjw,
                             hash_compare_strings, hash_freer);
      if (*ht == NULL)
        xalloc_die ();
    }

  return *ht;
}

/* Initialize *HT if it is NULL.
   Insert FILENAME into HT.  */
static void
AD_mark_helper (Hash_table **ht, char *filename)
{
  void *ent = hash_insert (AD_ensure_initialized (ht), filename);
  if (ent == NULL)
    xalloc_die ();
  else
    {
      if (ent != filename)
        free (filename);
    }
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
   is the result of calling lstat on DIR from the parent of DIR.
   Longjump out (skipping the entire command line argument we're
   dealing with) if `fstat (FD_CWD, ...' fails or if someone has
   replaced DIR with e.g., a symlink to some other directory.  */
static void
AD_push (int fd_cwd, Dirstack_state *ds, char const *dir,
         struct stat const *dir_sb_from_parent)
{
  struct AD_ent *top;

  push_dir (ds, dir);

  /* If our uses of openat are guaranteed not to
     follow a symlink, then we can skip this check.  */
  if (! HAVE_WORKING_O_NOFOLLOW)
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

  if (cycle_check (&ds->cycle_check_state, dir_sb_from_parent))
    {
      error (0, 0, _("\
WARNING: Circular directory structure.\n\
This almost certainly means that you have a corrupted file system.\n\
NOTIFY YOUR SYSTEM MANAGER.\n\
The following directory is part of the cycle:\n  %s\n"),
             quote (full_filename (".")));
      longjmp (ds->current_arg_jumpbuf, 1);
    }

  /* Extend the stack.  */
  obstack_blank (&ds->Active_dir, sizeof (struct AD_ent));

  /* The active directory stack must be one larger than the length stack.  */
  assert (AD_stack_height (ds) ==
          1 + obstack_object_size (&ds->len_stack) / sizeof (size_t));

  /* Fill in the new values.  */
  top = AD_stack_top (ds);
  top->dev_ino.st_dev = dir_sb_from_parent->st_dev;
  top->dev_ino.st_ino = dir_sb_from_parent->st_ino;
  top->unremovable = NULL;
}

static inline bool
AD_is_removable (Dirstack_state const *ds, char const *file)
{
  struct AD_ent *top = AD_stack_top (ds);
  return ! (top->unremovable && hash_lookup (top->unremovable, file));
}

/* Return 1 if FILE is an unwritable non-symlink,
   0 if it is writable or some other type of file,
   -1 and set errno if there is some problem in determining the answer.
   Set *BUF to the file status.
   This is to avoid calling euidaccess when FILE is a symlink.  */
static int
write_protected_non_symlink (int fd_cwd,
                             char const *file,
                             Dirstack_state const *ds,
                             struct stat *buf)
{
  if (can_write_any_file ())
    return 0;
  if (cache_fstatat (fd_cwd, file, buf, AT_SYMLINK_NOFOLLOW) != 0)
    return -1;
  if (S_ISLNK (buf->st_mode))
    return 0;
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

     3) if (euidaccess (xfull_filename (file), W_OK) == 0)
        Disadvantage: doesn't work if xfull_filename is too long.
        Inefficient for very deep trees (O(Depth^2)).

     4) If the full pathname is sufficiently short (say, less than
        PATH_MAX or 8192 bytes, whichever is shorter):
        use method (3) (i.e., euidaccess (xfull_filename (file), W_OK));
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
        use method (3) (i.e., euidaccess (xfull_filename (file), W_OK));
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

    if (MIN (PATH_MAX, 8192) <= file_name_len)
      return ! euidaccess_stat (buf, W_OK);
    if (euidaccess (xfull_filename (ds, file), W_OK) == 0)
      return 0;
    if (errno == EACCES)
      {
        errno = 0;
        return 1;
      }

    /* Perhaps some other process has removed the file, or perhaps this
       is a buggy NFS client.  */
    return -1;
  }
}

/* Prompt whether to remove FILENAME, if required via a combination of
   the options specified by X and/or file attributes.  If the file may
   be removed, return RM_OK.  If the user declines to remove the file,
   return RM_USER_DECLINED.  If not ignoring missing files and we
   cannot lstat FILENAME, then return RM_ERROR.

   *PDIRENT_TYPE is the type of the directory entry; update it to DT_DIR
   or DT_LNK as needed.  *SBUF is the file's status.

   Depending on MODE, ask whether to `descend into' or to `remove' the
   directory FILENAME.  MODE is ignored when FILENAME is not a directory.
   Set *IS_EMPTY to T_YES if FILENAME is an empty directory, and it is
   appropriate to try to remove it with rmdir (e.g. recursive mode).
   Don't even try to set *IS_EMPTY when MODE == PA_REMOVE_DIR.  */
static enum RM_status
prompt (int fd_cwd, Dirstack_state const *ds, char const *filename,
        int *pdirent_type, struct stat *sbuf,
        struct rm_options const *x, enum Prompt_action mode,
        Ternary *is_empty)
{
  int write_protected = 0;
  int dirent_type = *pdirent_type;

  *is_empty = T_UNKNOWN;

  if (x->interactive == RMI_NEVER)
    return RM_OK;

  int wp_errno = 0;

  if (!x->ignore_missing_files
      && ((x->interactive == RMI_ALWAYS) || x->stdin_tty)
      && dirent_type != DT_LNK)
    {
      write_protected = write_protected_non_symlink (fd_cwd, filename, ds, sbuf);
      wp_errno = errno;
    }

  if (write_protected || x->interactive == RMI_ALWAYS)
    {
      if (0 <= write_protected && dirent_type == DT_UNKNOWN)
        {
          if (cache_fstatat (fd_cwd, filename, sbuf, AT_SYMLINK_NOFOLLOW) == 0)
            {
              if (S_ISLNK (sbuf->st_mode))
                dirent_type = DT_LNK;
              else if (S_ISDIR (sbuf->st_mode))
                dirent_type = DT_DIR;
              /* Otherwise it doesn't matter, so leave it DT_UNKNOWN.  */
              *pdirent_type = dirent_type;
            }
          else
            {
              /* This happens, e.g., with `rm '''.  */
              write_protected = -1;
              wp_errno = errno;
            }
        }

      if (0 <= write_protected)
        switch (dirent_type)
          {
          case DT_LNK:
            /* Using permissions doesn't make sense for symlinks.  */
            if (x->interactive != RMI_ALWAYS)
              return RM_OK;
            break;

          case DT_DIR:
            if (!x->recursive)
              {
                write_protected = -1;
                wp_errno = EISDIR;
              }
            break;
          }

      char const *quoted_name = quote (full_filename (filename));

      if (write_protected < 0)
        {
          error (0, wp_errno, _("cannot remove %s"), quoted_name);
          return RM_ERROR;
        }

      /* Issue the prompt.  */
      /* FIXME: use a variant of error (instead of fprintf) that doesn't
         append a newline.  Then we won't have to declare program_name in
         this file.  */
      if (dirent_type == DT_DIR
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
          if (cache_fstatat (fd_cwd, filename, sbuf, AT_SYMLINK_NOFOLLOW) != 0)
            {
              error (0, errno, _("cannot remove %s"), quoted_name);
              return RM_ERROR;
            }

          fprintf (stderr,
                   (write_protected
                    /* TRANSLATORS: You may find it more convenient to
                       translate "%s: remove %s (write-protected) %s? "
                       instead.  It should avoid grammatical problems
                       with the output of file_type.  */
                    ? _("%s: remove write-protected %s %s? ")
                    : _("%s: remove %s %s? ")),
                   program_name, file_type (sbuf), quoted_name);
        }

      if (!yesno ())
        return RM_USER_DECLINED;
    }
  return RM_OK;
}

/* Return true if FILENAME is a directory (and not a symlink to a directory).
   Otherwise, including the case in which lstat fails, return false.
   *ST is FILENAME's tstatus.
   Do not modify errno.  */
static inline bool
is_dir_lstat (int fd_cwd, char const *filename, struct stat *st)
{
  int saved_errno = errno;
  bool is_dir =
    (cache_fstatat (fd_cwd, filename, st, AT_SYMLINK_NOFOLLOW) == 0
     && S_ISDIR (st->st_mode));
  errno = saved_errno;
  return is_dir;
}

/* Return true if FILENAME is a non-directory.
   Otherwise, including the case in which lstat fails, return false.
   *ST is FILENAME's tstatus.
   Do not modify errno.  */
static inline bool
is_nondir_lstat (int fd_cwd, char const *filename, struct stat *st)
{
  int saved_errno = errno;
  bool is_non_dir =
    (cache_fstatat (fd_cwd, filename, st, AT_SYMLINK_NOFOLLOW) == 0
     && !S_ISDIR (st->st_mode));
  errno = saved_errno;
  return is_non_dir;
}

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
      if (ignorable_missing (X, errno))					\
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
      if (ignorable_missing (X, errno))			\
        return RM_OK;					\
                                                        \
      if (errno == ENOTEMPTY || errno == EEXIST)	\
        return RM_NONEMPTY_DIR;				\
    }							\
  while (0)

/* When a function like unlink, rmdir, or fstatat fails with an errno
   value of ERRNUM, return true if the specified file system object
   is guaranteed not to exist;  otherwise, return false.  */
static inline bool
nonexistent_file_errno (int errnum)
{
  /* Do not include ELOOP here, since the specified file may indeed
     exist, but be (in)accessible only via too long a symlink chain.
     Likewise for ENAMETOOLONG, since rm -f ./././.../foo may fail
     if the "..." part expands to a long enough sequence of "./"s,
     even though ./foo does indeed exist.  */

  switch (errnum)
    {
    case ENOENT:
    case ENOTDIR:
      return true;
    default:
      return false;
    }
}

/* Encapsulate the test for whether the errno value, ERRNUM, is ignorable.  */
static inline bool
ignorable_missing (struct rm_options const *x, int errnum)
{
  return x->ignore_missing_files && nonexistent_file_errno (errnum);
}

/* Remove the file or directory specified by FILENAME.
   Return RM_OK if it is removed, and RM_ERROR or RM_USER_DECLINED if not.
   But if FILENAME specifies a non-empty directory, return RM_NONEMPTY_DIR. */

static enum RM_status
remove_entry (int fd_cwd, Dirstack_state const *ds, char const *filename,
              int dirent_type_arg, struct stat *st,
              struct rm_options const *x)
{
  Ternary is_empty_directory;
  enum RM_status s = prompt (fd_cwd, ds, filename, &dirent_type_arg, st, x,
                             PA_DESCEND_INTO_DIR,
                             &is_empty_directory);
  int dirent_type = dirent_type_arg;
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
      if (dirent_type == DT_DIR && ! x->recursive)
        {
          error (0, EISDIR, _("cannot remove %s"),
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
         unlink call.  If FILENAME is a command-line argument, then
         DIRENT_TYPE is DT_UNKNOWN so we'll first try to unlink it.
         Using unlink here is ok, because it cannot remove a
         directory.  */
      if (dirent_type == DT_DIR)
        return RM_NONEMPTY_DIR;

      DO_UNLINK (fd_cwd, filename, x);

      /* Upon a failed attempt to unlink a directory, most non GNU/Linux
         systems set errno to the POSIX-required value EPERM.  In that case,
         change errno to EISDIR so that we emit a better diagnostic.  */
      if (! x->recursive && errno == EPERM && is_dir_lstat (fd_cwd,
                                                            filename, st))
        errno = EISDIR;

      if (! x->recursive
          || (cache_stat_ok (st) && !S_ISDIR (st->st_mode))
          || ((errno == EACCES || errno == EPERM)
              && is_nondir_lstat (fd_cwd, filename, st))
          )
        {
          if (ignorable_missing (x, errno))
            return RM_OK;

          /* Either --recursive is not in effect, or the file cannot be a
             directory.  Report the unlink problem and fail.  */
          error (0, errno, _("cannot remove %s"),
                 quote (full_filename (filename)));
          return RM_ERROR;
        }
      assert (!cache_stat_ok (st) || S_ISDIR (st->st_mode));
    }
  else
    {
      /* If we don't already know whether FILENAME is a directory,
         find out now.  Then, if it's a non-directory, we can use
         unlink on it.  */

      if (dirent_type == DT_UNKNOWN)
        {
          if (fstatat (fd_cwd, filename, st, AT_SYMLINK_NOFOLLOW))
            {
              if (ignorable_missing (x, errno))
                return RM_OK;

              error (0, errno, _("cannot remove %s"),
                     quote (full_filename (filename)));
              return RM_ERROR;
            }

          if (S_ISDIR (st->st_mode))
            dirent_type = DT_DIR;
        }

      if (dirent_type != DT_DIR)
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
          error (0, EISDIR, _("cannot remove %s"),
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
   The caller must use a nonnnull CWD_ERRNO the first
   time this function is called for each command-line-specified directory.
   If CWD_ERRNO is not null, set *CWD_ERRNO to the appropriate error number
   if this function fails to restore the initial working directory.
   If it is null, report an error and exit if the working directory
   isn't restored.  */
static DIR *
fd_to_subdirp (int fd_cwd, char const *f,
               int prev_errno,
               struct stat *subdir_sb,
               int *cwd_errno ATTRIBUTE_UNUSED)
{
  int open_flags = O_RDONLY | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK;
  int fd_sub = openat_permissive (fd_cwd, f, open_flags, 0, cwd_errno);
  int saved_errno;

  /* Record dev/ino of F.  We may compare them against saved values
     to thwart any attempt to subvert the traversal.  They are also used
     to detect directory cycles.  */
  if (fd_sub < 0)
    return NULL;
  else if (fstat (fd_sub, subdir_sb) != 0)
    saved_errno = errno;
  else if (S_ISDIR (subdir_sb->st_mode))
    {
      DIR *subdir_dirp = fdopendir (fd_sub);
      if (subdir_dirp)
        return subdir_dirp;
      saved_errno = errno;
    }
  else
    saved_errno = (prev_errno ? prev_errno : ENOTDIR);

  close (fd_sub);
  errno = saved_errno;
  return NULL;
}

struct readdir_data
{
  ino_t ino;
  char name[FLEXIBLE_ARRAY_MEMBER];
};

#if HAVE_STRUCT_DIRENT_D_TYPE
/* A comparison function to sort on increasing inode number.  */
static int
compare_ino (void const *av, void const *bv)
{
  struct readdir_data const *const *a = av;
  struct readdir_data const *const *b = bv;
  return (a[0]->ino < b[0]->ino ? -1
          : b[0]->ino < a[0]->ino ? 1 : 0);
}

/* Return an approximation of the maximum number of dirent entries
   in a directory with stat data *ST.  */
static size_t
dirent_count (struct stat const *st)
{
  return st->st_size / 16;
}

#if defined __linux__ \
  && HAVE_SYS_VFS_H && HAVE_FSTATFS && HAVE_STRUCT_STATFS_F_TYPE
#  include <sys/vfs.h>
#  include "fs.h"

/* Return false if it is easy to determine the file system type of
   the directory on which DIR_FD is open, and sorting dirents on
   inode numbers is known not to improve traversal performance with
   that type of file system.  Otherwise, return true.  */
static bool
dirent_inode_sort_may_be_useful (int dir_fd)
{
  /* Skip the sort only if we can determine efficiently
     that skipping it is the right thing to do.
     The cost of performing an unnecessary sort is negligible,
     while the cost of *not* performing it can be O(N^2) with
     a very large constant.  */
  struct statfs fs_buf;

  /* If fstatfs fails, assume sorting would be useful.  */
  if (fstatfs (dir_fd, &fs_buf) != 0)
    return true;

  /* FIXME: what about when f_type is not an integral type?
     deal with that if/when it's encountered.  */
  switch (fs_buf.f_type)
    {
    case S_MAGIC_TMPFS:
    case S_MAGIC_NFS:
      /* On a file system of any of these types, sorting
         is unnecessary, and hence wasteful.  */
      return false;

    default:
      return true;
    }
}
# else /* !HAVE_STRUCT_STATFS_F_TYPE */
static bool dirent_inode_sort_may_be_useful (int dir_fd ATTRIBUTE_UNUSED)
{
  return true;
}
# endif /* !HAVE_STRUCT_STATFS_F_TYPE */
#endif /* HAVE_STRUCT_DIRENT_D_TYPE */

/* When a directory contains very many entries, operating on N entries in
   readdir order can be very seek-intensive (be it to unlink or even to
   merely stat each entry), to the point that it results in O(N^2) work.
   This is file-system-specific: ext3 and ext4 (as of 2008) are susceptible,
   but tmpfs is not.  The general solution is to process entries in inode
   order.  That means reading all entries, and sorting them before operating
   on any.  As such, it is useful only on systems with useful dirent.d_ino.
   Since 'rm -r's removal process must traverse into directories and since
   this preprocessing phase can allocate O(N) storage, here we store and
   sort only non-directory entries, and then remove all of those, so that we
   can free all allocated storage before traversing into any subdirectory.
   Perform this optimization only when not interactive and not in verbose
   mode, to keep the implementation simple and to minimize duplication.
   Upon failure, simply free any resources and return.  */
static void
preprocess_dir (DIR **dirp, struct rm_options const *x)
{
#if HAVE_STRUCT_DIRENT_D_TYPE

  struct stat st;
  /* There are many reasons to return right away, skipping this
     optimization.  The penalty for being wrong is that we will
     perform a small amount of extra work.

     Skip this optimization if... */

  int dir_fd = dirfd (*dirp);
  if (/* - there is a chance of interactivity */
      x->interactive != RMI_NEVER

      /* - we're in verbose mode */
      || x->verbose

      /* - privileged users can unlink nonempty directories.
         Otherwise, there'd be a race condition between the readdir
         call (in which we learn dirent.d_type) and the unlink, by
         which time the non-directory may be replaced with a directory. */
      || ! cannot_unlink_dir ()

      /* - we can't fstat the file descriptor */
      || fstat (dir_fd, &st) != 0

      /* - the directory is smaller than some threshold.
         Estimate the number of inodes with a heuristic.
         There's no significant benefit to sorting if there are
         too few inodes.  */
      || dirent_count (&st) < INODE_SORT_DIR_ENTRIES_THRESHOLD

      /* Sort only if it might help.  */
      || ! dirent_inode_sort_may_be_useful (dir_fd))
    return;

  /* FIXME: maybe test file system type, too; skip if it's tmpfs: see fts.c */

  struct obstack o_readdir_data;  /* readdir data: inode,name pairs  */
  struct obstack o_p;  /* an array of pointers to each inode,name pair */

  /* Arrange to longjmp upon obstack allocation failure.  */
  jmp_buf readdir_jumpbuf;
  if (setjmp (readdir_jumpbuf))
    goto cleanup;

  obstack_init_minimal (&o_readdir_data);
  obstack_init_minimal (&o_p);

  obstack_specify_allocation_with_arg (&o_readdir_data, 0, 0,
                                       rm_malloc, rm_free, &readdir_jumpbuf);
  obstack_specify_allocation_with_arg (&o_p, 0, 0,
                                       rm_malloc, rm_free, &readdir_jumpbuf);

  /* Read all entries, storing <d_ino, d_name> for each non-dir one.
     Maintain a parallel list of pointers into the primary buffer.  */
  while (1)
    {
      struct dirent const *dp;
      dp = readdir_ignoring_dot_and_dotdot (*dirp);
      /* no need to distinguish EOF from failure */
      if (dp == NULL)
        break;

      /* Skip known-directory and type-unknown entries.  */
      if (D_TYPE (dp) == DT_UNKNOWN || D_TYPE (dp) == DT_DIR)
        break;

      size_t name_len = strlen (dp->d_name);
      size_t ent_len = offsetof (struct readdir_data, name) + name_len + 1;
      struct readdir_data *v = obstack_alloc (&o_readdir_data, ent_len);
      v->ino = D_INO (dp);
      memcpy (v->name, dp->d_name, name_len + 1);

      /* Append V to the list of pointers.  */
      obstack_ptr_grow (&o_p, v);
    }

  /* Compute size and finalize VV.  */
  size_t n = obstack_object_size (&o_p) / sizeof (void *);
  struct readdir_data **vv = obstack_finish (&o_p);

  /* Sort on inode number.  */
  qsort(vv, n, sizeof *vv, compare_ino);

  /* Iterate through those pointers, unlinking each name.  */
  for (size_t i = 0; i < n; i++)
    {
      /* ignore failure */
      unlinkat (dir_fd, vv[i]->name, 0);
    }

 cleanup:
  obstack_free (&o_readdir_data, NULL);
  obstack_free (&o_p, NULL);
  rewinddir (*dirp);
#endif
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

  /* This is just an optimization.
     It's not a fatal problem if it fails.  */
  preprocess_dir (dirp, x);

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
          else if (NEED_REWIND (n_unlinked_since_opendir_or_last_rewind))
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
      cache_stat_init (subdir_sb);
      tmp_status = remove_entry (dirfd (*dirp), ds, f,
                                 D_TYPE (dp), subdir_sb, x);
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
                                              errno, subdir_sb, NULL);
            if (subdir_dirp == NULL)
              {
                status = RM_ERROR;

                /* CAUTION: this test and diagnostic are identical to
                   those following the other use of fd_to_subdirp.  */
                if (ignorable_missing (x, errno))
                  {
                    /* With -f, don't report "file not found".  */
                  }
                else
                  {
                    /* Upon fd_to_subdirp failure, try to remove F directly,
                       in case it's just an empty directory.  */
                    int saved_errno = errno;
                    if (unlinkat (dirfd (*dirp), f, AT_REMOVEDIR) == 0)
                      status = RM_OK;
                    else
                      error (0, saved_errno,
                             _("cannot remove %s"), quote (full_filename (f)));
                  }

                if (status == RM_ERROR)
                  AD_mark_as_unremovable (ds, f);
                break;
              }

            *subdir = xstrdup (f);
            if (closedir (*dirp) != 0)
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
            struct stat *dir_st,
            struct rm_options const *x, int *cwd_errno)
{
  enum RM_status status;
  dev_t current_dev = dir_st->st_dev;

  /* There is a race condition in that an attacker could replace the nonempty
     directory, DIR, with a symlink between the preceding call to rmdir
     (unlinkat, in our caller) and fd_to_subdirp's openat call.  But on most
     systems, even those without openat, this isn't a problem, since we ensure
     that opening a symlink will fail, when that is possible.  Otherwise,
     fd_to_subdirp's fstat, along with the `fstat' and the dev/ino
     comparison in AD_push ensure that we detect it and fail.  */

  DIR *dirp = fd_to_subdirp (fd_cwd, dir, 0, dir_st, cwd_errno);

  if (dirp == NULL)
    {
      /* CAUTION: this test and diagnostic are identical to
         those following the other use of fd_to_subdirp.  */
      if (ignorable_missing (x, errno))
        {
          /* With -f, don't report "file not found".  */
        }
      else
        {
          /* Upon fd_to_subdirp failure, try to remove DIR directly,
             in case it's just an empty directory.  */
          int saved_errno = errno;
          if (unlinkat (fd_cwd, dir, AT_REMOVEDIR) == 0)
            return RM_OK;

          error (0, saved_errno,
                 _("cannot remove %s"), quote (full_filename (dir)));
        }

      return RM_ERROR;
    }

  if (ROOT_DEV_INO_CHECK (x->root_dev_ino, dir_st))
    {
      ROOT_DEV_INO_WARN (full_filename (dir));
      status = RM_ERROR;
      goto closedir_and_return;
    }

  AD_push (dirfd (dirp), ds, dir, dir_st);
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
          if ( ! x->one_file_system
               || subdir_sb.st_dev == current_dev)
            {
              AD_push (dirfd (dirp), ds, subdir, &subdir_sb);
              AD_INIT_OTHER_MEMBERS ();
              free (subdir);
              continue;
            }

          /* Here, --one-file-system is in effect, and with remove_cwd_entries'
             traversal into the current directory, (known as SUBDIR, from ..),
             DIRP's device number is different from CURRENT_DEV.  Arrange not
             to do anything more with this hierarchy.  */
          error (0, 0, _("skipping %s, since it's on a different device"),
                 quote (full_filename (subdir)));
          free (subdir);
          AD_mark_current_as_unremovable (ds);
          tmp_status = RM_ERROR;
          UPDATE_STATUS (status, tmp_status);
        }

      /* Execution reaches this point when we've removed the last
         removable entry from the current directory -- or, with
         --one-file-system, when the current directory is on a
         different file system.  */
      {
        int fd;
        /* The name of the directory that we have just processed,
           nominally removing all of its contents.  */
        char *empty_dir = AD_pop_and_chdir (dirp, &fd, ds);
        dirp = NULL;
        assert (fd != AT_FDCWD || AD_stack_height (ds) == 1);

        /* Try to remove EMPTY_DIR only if remove_cwd_entries succeeded.  */
        if (tmp_status == RM_OK)
          {
            struct stat empty_st;
            Ternary is_empty;
            int dirent_type = DT_DIR;
            enum RM_status s = prompt (fd, ds, empty_dir, &dirent_type,
                                       cache_stat_init (&empty_st), x,
                                       PA_REMOVE_DIR, &is_empty);

            if (s != RM_OK)
              {
                free (empty_dir);
                status = s;
                if (fd != AT_FDCWD)
                  close (fd);
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

        if (fd == AT_FDCWD)
          break;

        dirp = fdopendir (fd);
        if (dirp == NULL)
          {
            error (0, errno, _("FATAL: cannot return to .. from %s"),
                   quote (full_filename (".")));
            close (fd);
            longjmp (ds->current_arg_jumpbuf, 1);
          }
      }
    }

  /* If the first/final hash table of unremovable entries was used,
     free it here.  */
  AD_stack_pop (ds);

 closedir_and_return:;
  if (dirp != NULL && closedir (dirp) != 0)
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
      struct rm_options const *x, int *cwd_errno)
{
  char const *base = last_component (filename);
  if (dot_or_dotdot (base))
    {
      error (0, 0, _(base == filename
                     ? N_("cannot remove directory %s")
                     : N_("cannot remove %s directory %s")),
             quote_n (0, base), quote_n (1, filename));
      return RM_ERROR;
    }

  struct stat st;
  cache_stat_init (&st);
  cycle_check_init (&ds->cycle_check_state);
  if (x->root_dev_ino)
    {
      if (cache_fstatat (AT_FDCWD, filename, &st, AT_SYMLINK_NOFOLLOW) != 0)
        {
          if (ignorable_missing (x, errno))
            return RM_OK;
          error (0, errno, _("cannot remove %s"), quote (filename));
          return RM_ERROR;
        }
      if (SAME_INODE (st, *(x->root_dev_ino)))
        {
          error (0, 0, _("cannot remove root directory %s"), quote (filename));
          return RM_ERROR;
        }
    }

  AD_push_initial (ds);
  AD_INIT_OTHER_MEMBERS ();

  enum RM_status status = remove_entry (AT_FDCWD, ds, filename,
                                        DT_UNKNOWN, &st, x);
  if (status == RM_NONEMPTY_DIR)
    {
      /* In the event that remove_dir->remove_cwd_entries detects
         a directory cycle, arrange to fail, give up on this FILE, but
         continue on with any other arguments.  */
      if (setjmp (ds->current_arg_jumpbuf))
        status = RM_ERROR;
      else
        status = remove_dir (AT_FDCWD, ds, filename, &st, x, cwd_errno);

      AD_stack_clear (ds);
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
  Dirstack_state ds;
  int cwd_errno = 0;
  size_t i;

  /* Arrange for obstack allocation failure to longjmp.  */
  if (setjmp (ds.current_arg_jumpbuf))
    {
      status = RM_ERROR;
      goto cleanup;
    }

  ds_init (&ds);

  for (i = 0; i < n_files; i++)
    {
      if (cwd_errno && IS_RELATIVE_FILE_NAME (file[i]))
        {
          error (0, 0, _("cannot remove relative-named %s"), quote (file[i]));
          status = RM_ERROR;
        }
      else
        {
          enum RM_status s = rm_1 (&ds, file[i], x, &cwd_errno);
          assert (VALID_STATUS (s));
          UPDATE_STATUS (status, s);
        }
    }

  if (x->require_restore_cwd && cwd_errno)
    {
      error (0, cwd_errno,
             _("cannot restore current working directory"));
      status = RM_ERROR;
    }

 cleanup:;
  ds_free (&ds);

  return status;
}
