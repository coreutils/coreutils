/* copy.c -- core functions for copying files and directories
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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Extracted from cp.c and librarified by Jim Meyering.  */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

#if HAVE_HURD_H
# include <hurd.h>
#endif

#include "system.h"
#include "error.h"
#include "backupfile.h"
#include "savedir.h"
#include "copy.h"
#include "cp-hash.h"
#include "hash.h"
#include "hash-pjw.h"
#include "same.h"
#include "dirname.h"
#include "full-write.h"
#include "path-concat.h"
#include "quote.h"
#include "same.h"
#include "xreadlink.h"

#define DO_CHOWN(Chown, File, New_uid, New_gid)				\
  (Chown (File, New_uid, New_gid)					\
   /* If non-root uses -p, it's ok if we can't preserve ownership.	\
      But root probably wants to know, e.g. if NFS disallows it,	\
      or if the target system doesn't support file ownership.  */	\
   && ((errno != EPERM && errno != EINVAL) || x->myeuid == 0))

#define SAME_OWNER(A, B) ((A).st_uid == (B).st_uid)
#define SAME_GROUP(A, B) ((A).st_gid == (B).st_gid)
#define SAME_OWNER_AND_GROUP(A, B) (SAME_OWNER (A, B) && SAME_GROUP (A, B))

#define UNWRITABLE(File_name, File_mode)		\
  ( /* euidaccess is not meaningful for symlinks */	\
    ! S_ISLNK (File_mode)				\
    && euidaccess (File_name, W_OK) != 0)

struct dir_list
{
  struct dir_list *parent;
  ino_t ino;
  dev_t dev;
};

/* Describe a just-created or just-renamed destination file.  */
struct F_triple
{
  char const* name;
  ino_t st_ino;
  dev_t st_dev;
};

/* Initial size of the above hash table.  */
#define DEST_INFO_INITIAL_CAPACITY 61

int euidaccess ();
int yesno ();

static int copy_internal (const char *src_path, const char *dst_path,
			  int new_dst, dev_t device,
			  struct dir_list *ancestors,
			  const struct cp_options *x,
			  int command_line_arg,
			  int *copy_into_self,
			  int *rename_succeeded);

/* Pointers to the file names:  they're used in the diagnostic that is issued
   when we detect the user is trying to copy a directory into itself.  */
static char const *top_level_src_path;
static char const *top_level_dst_path;

/* The invocation name of this program.  */
extern char *program_name;

/* Encapsulate selection of the file mode to be applied to
   new non-directories.  */

static mode_t
get_dest_mode (const struct cp_options *option, mode_t mode)
{
  /* In some applications (e.g., install), use precisely the
     specified mode.  */
  if (option->set_mode)
    return option->mode;

  /* Honor the umask for `cp', but not for `mv' or `cp -p'.
     In addition, `cp' without -p must clear the set-user-ID and set-group-ID
     bits.  POSIX requires it do that when creating new files.  */
  if (!option->move_mode && !option->preserve_mode)
    mode &= (option->umask_kill & ~(S_ISUID | S_ISGID));

  return mode;
}

/* FIXME: describe */
/* FIXME: rewrite this to use a hash table so we avoid the quadratic
   performance hit that's probably noticeable only on trees deeper
   than a few hundred levels.  See use of active_dir_map in remove.c  */

static int
is_ancestor (const struct stat *sb, const struct dir_list *ancestors)
{
  while (ancestors != 0)
    {
      if (ancestors->ino == sb->st_ino && ancestors->dev == sb->st_dev)
	return 1;
      ancestors = ancestors->parent;
    }
  return 0;
}

/* Read the contents of the directory SRC_PATH_IN, and recursively
   copy the contents to DST_PATH_IN.  NEW_DST is nonzero if
   DST_PATH_IN is a directory that was created previously in the
   recursion.   SRC_SB and ANCESTORS describe SRC_PATH_IN.
   Set *COPY_INTO_SELF to nonzero if SRC_PATH_IN is a parent of
   (or the same as) DST_PATH_IN;  otherwise, set it to zero.
   Return 0 if successful, -1 if an error occurs. */

static int
copy_dir (const char *src_path_in, const char *dst_path_in, int new_dst,
	  const struct stat *src_sb, struct dir_list *ancestors,
	  const struct cp_options *x, int *copy_into_self)
{
  char *name_space;
  char *namep;
  struct cp_options non_command_line_options = *x;
  int ret = 0;

  name_space = savedir (src_path_in);
  if (name_space == NULL)
    {
      /* This diagnostic is a bit vague because savedir can fail in
         several different ways.  */
      error (0, errno, _("cannot access %s"), quote (src_path_in));
      return -1;
    }

  /* For cp's -H option, dereference command line arguments, but do not
     dereference symlinks that are found via recursive traversal.  */
  if (x->dereference == DEREF_COMMAND_LINE_ARGUMENTS)
    non_command_line_options.xstat = lstat;

  namep = name_space;
  while (*namep != '\0')
    {
      int local_copy_into_self;
      char *src_path = path_concat (src_path_in, namep, NULL);
      char *dst_path = path_concat (dst_path_in, namep, NULL);

      if (dst_path == NULL || src_path == NULL)
	xalloc_die ();

      ret |= copy_internal (src_path, dst_path, new_dst, src_sb->st_dev,
			    ancestors, &non_command_line_options, 0,
			    &local_copy_into_self, NULL);
      *copy_into_self |= local_copy_into_self;

      free (dst_path);
      free (src_path);

      namep += strlen (namep) + 1;
    }
  free (name_space);
  return -ret;
}

/* Copy a regular file from SRC_PATH to DST_PATH.
   If the source file contains holes, copies holes and blocks of zeros
   in the source file as holes in the destination file.
   (Holes are read as zeroes by the `read' system call.)
   Use DST_MODE as the 3rd argument in the call to open.
   X provides many option settings.
   Return 0 if successful, -1 if an error occurred.
   *NEW_DST is as in copy_internal.  SRC_SB is the result
   of calling xstat (aka stat in this case) on SRC_PATH.  */

static int
copy_reg (const char *src_path, const char *dst_path,
	  const struct cp_options *x, mode_t dst_mode, int *new_dst,
	  struct stat const *src_sb)
{
  char *buf;
  int buf_size;
  int dest_desc;
  int source_desc;
  struct stat sb;
  struct stat src_open_sb;
  char *cp;
  int *ip;
  int return_val = 0;
  off_t n_read_total = 0;
  int last_write_made_hole = 0;
  int make_holes = (x->sparse_mode == SPARSE_ALWAYS);

  source_desc = open (src_path, O_RDONLY);
  if (source_desc < 0)
    {
      error (0, errno, _("cannot open %s for reading"), quote (src_path));
      return -1;
    }

  if (fstat (source_desc, &src_open_sb))
    {
      error (0, errno, _("cannot fstat %s"), quote (src_path));
      return_val = -1;
      goto close_src_desc;
    }

  /* Compare the source dev/ino from the open file to the incoming,
     saved ones obtained via a previous call to stat.  */
  if (! SAME_INODE (*src_sb, src_open_sb))
    {
      error (0, 0,
	     _("skipping file %s, as it was replaced while being copied"),
	     quote (src_path));
      return_val = -1;
      goto close_src_desc;
    }

  /* These semantics are required for cp.
     The if-block will be taken in move_mode.  */
  if (*new_dst)
    {
      dest_desc = open (dst_path, O_WRONLY | O_CREAT, dst_mode);
    }
  else
    {
      dest_desc = open (dst_path, O_WRONLY | O_TRUNC, dst_mode);

      if (dest_desc < 0 && x->unlink_dest_after_failed_open)
	{
	  if (unlink (dst_path))
	    {
	      error (0, errno, _("cannot remove %s"), quote (dst_path));
	      return_val = -1;
	      goto close_src_desc;
	    }

	  /* Tell caller that the destination file was unlinked.  */
	  *new_dst = 1;

	  /* Try the open again, but this time with different flags.  */
	  dest_desc = open (dst_path, O_WRONLY | O_CREAT, dst_mode);
	}
    }

  if (dest_desc < 0)
    {
      error (0, errno, _("cannot create regular file %s"), quote (dst_path));
      return_val = -1;
      goto close_src_desc;
    }

  /* Determine the optimal buffer size.  */

  if (fstat (dest_desc, &sb))
    {
      error (0, errno, _("cannot fstat %s"), quote (dst_path));
      return_val = -1;
      goto close_src_and_dst_desc;
    }

  buf_size = ST_BLKSIZE (sb);

#if HAVE_STRUCT_STAT_ST_BLOCKS
  if (x->sparse_mode == SPARSE_AUTO && S_ISREG (sb.st_mode))
    {
      /* Use a heuristic to determine whether SRC_PATH contains any
	 sparse blocks. */

      if (fstat (source_desc, &sb))
	{
	  error (0, errno, _("cannot fstat %s"), quote (src_path));
	  return_val = -1;
	  goto close_src_and_dst_desc;
	}

      /* If the file has fewer blocks than would normally
	 be needed for a file of its size, then
	 at least one of the blocks in the file is a hole. */
      if (S_ISREG (sb.st_mode)
	  && sb.st_size / ST_NBLOCKSIZE > ST_NBLOCKS (sb))
	make_holes = 1;
    }
#endif

  /* Make a buffer with space for a sentinel at the end.  */

  buf = (char *) alloca (buf_size + sizeof (int));

  for (;;)
    {
      ssize_t n_read = read (source_desc, buf, buf_size);
      if (n_read < 0)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
#endif
	  error (0, errno, _("reading %s"), quote (src_path));
	  return_val = -1;
	  goto close_src_and_dst_desc;
	}
      if (n_read == 0)
	break;

      n_read_total += n_read;

      ip = 0;
      if (make_holes)
	{
	  buf[n_read] = 1;	/* Sentinel to stop loop.  */

	  /* Find first nonzero *word*, or the word with the sentinel.  */

	  ip = (int *) buf;
	  while (*ip++ == 0)
	    ;

	  /* Find the first nonzero *byte*, or the sentinel.  */

	  cp = (char *) (ip - 1);
	  while (*cp++ == 0)
	    ;

	  /* If we found the sentinel, the whole input block was zero,
	     and we can make a hole.  */

	  if (cp > buf + n_read)
	    {
	      /* Make a hole.  */
	      if (lseek (dest_desc, (off_t) n_read, SEEK_CUR) < 0L)
		{
		  error (0, errno, _("cannot lseek %s"), quote (dst_path));
		  return_val = -1;
		  goto close_src_and_dst_desc;
		}
	      last_write_made_hole = 1;
	    }
	  else
	    /* Clear to indicate that a normal write is needed. */
	    ip = 0;
	}
      if (ip == 0)
	{
	  size_t n = n_read;
	  if (full_write (dest_desc, buf, n) != n)
	    {
	      error (0, errno, _("writing %s"), quote (dst_path));
	      return_val = -1;
	      goto close_src_and_dst_desc;
	    }
	  last_write_made_hole = 0;
	}
    }

  /* If the file ends with a `hole', something needs to be written at
     the end.  Otherwise the kernel would truncate the file at the end
     of the last write operation.  */

  if (last_write_made_hole)
    {
#if HAVE_FTRUNCATE
      /* Write a null character and truncate it again.  */
      if (full_write (dest_desc, "", 1) != 1
	  || ftruncate (dest_desc, n_read_total) < 0)
#else
      /* Seek backwards one character and write a null.  */
      if (lseek (dest_desc, (off_t) -1, SEEK_CUR) < 0L
	  || full_write (dest_desc, "", 1) != 1)
#endif
	{
	  error (0, errno, _("writing %s"), quote (dst_path));
	  return_val = -1;
	}
    }

close_src_and_dst_desc:
  if (close (dest_desc) < 0)
    {
      error (0, errno, _("closing %s"), quote (dst_path));
      return_val = -1;
    }
close_src_desc:
  if (close (source_desc) < 0)
    {
      error (0, errno, _("closing %s"), quote (src_path));
      return_val = -1;
    }

  return return_val;
}

/* Return nonzero if it's ok that the source and destination
   files are the `same' by some measure.  The goal is to avoid
   making the `copy' operation remove both copies of the file
   in that case, while still allowing the user to e.g., move or
   copy a regular file onto a symlink that points to it.
   Try to minimize the cost of this function in the common case.  */

static int
same_file_ok (const char *src_path, const struct stat *src_sb,
	      const char *dst_path, const struct stat *dst_sb,
	      const struct cp_options *x, int *return_now)
{
  const struct stat *src_sb_link;
  const struct stat *dst_sb_link;
  struct stat tmp_dst_sb;
  struct stat tmp_src_sb;

  int same_link;
  int same = (SAME_INODE (*src_sb, *dst_sb));

  *return_now = 0;

  /* FIXME: this should (at the very least) be moved into the following
     if-block.  More likely, it should be removed, because it inhibits
     making backups.  But removing it will result in a change in behavior
     that will probably have to be documented -- and tests will have to
     be updated.  */
  if (same && x->hard_link)
    {
      *return_now = 1;
      return 1;
    }

  if (x->xstat == lstat)
    {
      same_link = same;

      /* If both the source and destination files are symlinks (and we'll
	 know this here IFF preserving symlinks (aka xstat == lstat),
	 then it's ok -- as long as they are distinct.  */
      if (S_ISLNK (src_sb->st_mode) && S_ISLNK (dst_sb->st_mode))
	return ! same_name (src_path, dst_path);

      src_sb_link = src_sb;
      dst_sb_link = dst_sb;
    }
  else
    {
      if (!same)
	return 1;

      if (lstat (dst_path, &tmp_dst_sb)
	  || lstat (src_path, &tmp_src_sb))
	return 1;

      src_sb_link = &tmp_src_sb;
      dst_sb_link = &tmp_dst_sb;

      same_link = SAME_INODE (*src_sb_link, *dst_sb_link);

      /* If both are symlinks, then it's ok, but only if the destination
	 will be unlinked before being opened.  This is like the test
	 above, but with the addition of the unlink_dest_before_opening
	 conjunct because otherwise, with two symlinks to the same target,
	 we'd end up truncating the source file.  */
      if (S_ISLNK (src_sb_link->st_mode) && S_ISLNK (dst_sb_link->st_mode)
	  && x->unlink_dest_before_opening)
	return 1;
    }

  /* The backup code ensures there's a copy, so it's usually ok to
     remove any destination file.  One exception is when both
     source and destination are the same directory entry.  In that
     case, moving the destination file aside (in making the backup)
     would also rename the source file and result in an error.  */
  if (x->backup_type != none)
    {
      if (!same_link)
	{
	  /* In copy mode when dereferencing symlinks, if the source is a
	     symlink and the dest is not, then backing up the destination
	     (moving it aside) would make it a dangling symlink, and the
	     subsequent attempt to open it in copy_reg would fail with
	     a misleading diagnostic.  Avoid that by returning zero in
	     that case so the caller can make cp (or mv when it has to
	     resort to reading the source file) fail now.  */

	  /* FIXME-note: even with the following kludge, we can still provoke
	     the offending diagnostic.  It's just a little harder to do :-)
	     $ rm -f a b c; touch c; ln -s c b; ln -s b a; cp -b a b
	     cp: cannot open `a' for reading: No such file or directory
	     That's misleading, since a subsequent `ls' shows that `a'
	     is still there.
	     One solution would be to open the source file *before* moving
	     aside the destination, but that'd involve a big rewrite. */
	  if ( ! x->move_mode
	       && x->dereference != DEREF_NEVER
	       && S_ISLNK (src_sb_link->st_mode)
	       && ! S_ISLNK (dst_sb_link->st_mode))
	    return 0;

	  return 1;
	}

      return ! same_name (src_path, dst_path);
    }

#if 0
  /* FIXME: use or remove */

  /* If we're making a backup, we'll detect the problem case in
     copy_reg because SRC_PATH will no longer exist.  Allowing
     the test to be deferred lets cp do some useful things.
     But when creating hardlinks and SRC_PATH is a symlink
     but DST_PATH is not we must test anyway.  */
  if (x->hard_link
      || !S_ISLNK (src_sb_link->st_mode)
      || S_ISLNK (dst_sb_link->st_mode))
    return 1;

  if (x->dereference != DEREF_NEVER)
    return 1;
#endif

  /* They may refer to the same file if we're in move mode and the
     target is a symlink.  That is ok, since we remove any existing
     destination file before opening it -- via `rename' if they're on
     the same file system, via `unlink (DST_PATH)' otherwise.
     It's also ok if they're distinct hard links to the same file.  */
  if ((x->move_mode || x->unlink_dest_before_opening)
      && (S_ISLNK (dst_sb_link->st_mode)
	  || (same_link && !same_name (src_path, dst_path))))
    return 1;

  /* If neither is a symlink, then it's ok as long as they aren't
     hard links to the same file.  */
  if (!S_ISLNK (src_sb_link->st_mode) && !S_ISLNK (dst_sb_link->st_mode))
    {
      if (!SAME_INODE (*src_sb_link, *dst_sb_link))
	return 1;

      /* If they are the same file, it's ok if we're making hard links.  */
      if (x->hard_link)
	{
	  *return_now = 1;
	  return 1;
	}
    }

  /* It's ok to remove a destination symlink.  But that works only when we
     unlink before opening the destination and when the source and destination
     files are on the same partition.  */
  if (x->unlink_dest_before_opening
      && S_ISLNK (dst_sb_link->st_mode))
    return dst_sb_link->st_dev == src_sb_link->st_dev;

  if (x->xstat == lstat)
    {
      if ( ! S_ISLNK (src_sb_link->st_mode))
	tmp_src_sb = *src_sb_link;
      else if (stat (src_path, &tmp_src_sb))
	return 1;

      if ( ! S_ISLNK (dst_sb_link->st_mode))
	tmp_dst_sb = *dst_sb_link;
      else if (stat (dst_path, &tmp_dst_sb))
	return 1;

      if ( ! SAME_INODE (tmp_src_sb, tmp_dst_sb))
	return 1;

      /* FIXME: shouldn't this be testing whether we're making symlinks?  */
      if (x->hard_link)
	{
	  *return_now = 1;
	  return 1;
	}
    }

  return 0;
}

static void
overwrite_prompt (char const *dst_path, struct stat const *dst_sb)
{
  if (euidaccess (dst_path, W_OK) != 0)
    {
      fprintf (stderr,
	       _("%s: overwrite %s, overriding mode %04lo? "),
	       program_name, quote (dst_path),
	       (unsigned long) (dst_sb->st_mode & CHMOD_MODE_BITS));
    }
  else
    {
      fprintf (stderr, _("%s: overwrite %s? "),
	       program_name, quote (dst_path));
    }
}

/* Hash an F_triple.  */
static unsigned int
triple_hash (void const *x, unsigned int table_size)
{
  struct F_triple const *p = x;

  /* Also take the name into account, so that when moving N hard links to the
     same file (all listed on the command line) all into the same directory,
     we don't experience any N^2 behavior.  */
  /* FIXME-maybe: is it worth the overhead of doing this
     just to avoid N^2 in such an unusual case?  N would have
     to be very large to make the N^2 factor noticable, and
     one would probably encounter a limit on the length of
     a command line before it became a problem.  */
  unsigned int tmp = hash_pjw (p->name, table_size);

  /* Ignoring the device number here should be fine.  */
  return (tmp | p->st_ino) % table_size;
}

/* Hash an F_triple.  */
static unsigned int
triple_hash_no_name (void const *x, unsigned int table_size)
{
  struct F_triple const *p = x;

  /* Ignoring the device number here should be fine.  */
  return p->st_ino % table_size;
}

/* Compare two F_triple structs.  */
static bool
triple_compare (void const *x, void const *y)
{
  struct F_triple const *a = x;
  struct F_triple const *b = y;
  return (SAME_INODE (*a, *b) && same_name (a->name, b->name)) ? true : false;
}

/* Free an F_triple.  */
static void
triple_free (void *x)
{
  struct F_triple *a = x;
  free ((char *) (a->name));
  free (a);
}

/* Initialize the hash table implementing a set of F_triple entries
   corresponding to destination files.  */
void
dest_info_init (struct cp_options *x)
{
  x->dest_info
    = hash_initialize (DEST_INFO_INITIAL_CAPACITY,
		       NULL,
		       triple_hash,
		       triple_compare,
		       triple_free);
}

/* Initialize the hash table implementing a set of F_triple entries
   corresponding to source files listed on the command line.  */
void
src_info_init (struct cp_options *x)
{

  /* Note that we use triple_hash_no_name here.
     Contrast with the use of triple_hash above.
     That is necessary because a source file may be specified
     in many different ways.  We want to warn about this
       cp a a d/
     as well as this:
       cp a ./a d/
  */
  x->src_info
    = hash_initialize (DEST_INFO_INITIAL_CAPACITY,
		       NULL,
		       triple_hash_no_name,
		       triple_compare,
		       triple_free);
}

/* Return nonzero if there is an entry in hash table, HT,
   for the file described by FILENAME and STATS.
   Otherwise, return zero.  */
static int
seen_file (Hash_table const *ht, char const *filename,
	   struct stat const *stats)
{
  struct F_triple new_ent;

  if (ht == NULL)
    return 0;

  new_ent.name = filename;
  new_ent.st_ino = stats->st_ino;
  new_ent.st_dev = stats->st_dev;

  return !!hash_lookup (ht, &new_ent);
}

/* Record destination filename, FILENAME, and dev/ino from *STATS,
   in the hash table, HT.  If HT is NULL, return immediately.
   If STATS is NULL, call lstat on FILENAME to get the device
   and inode numbers.  If that lstat fails, simply return.
   If memory allocation fails, exit immediately.  */
static void
record_file (Hash_table *ht, char const *filename,
	     struct stat const *stats)
{
  struct F_triple *ent;

  if (ht == NULL)
    return;

  ent = (struct F_triple *) xmalloc (sizeof *ent);
  ent->name = xstrdup (filename);
  if (stats)
    {
      ent->st_ino = stats->st_ino;
      ent->st_dev = stats->st_dev;
    }
  else
    {
      struct stat sb;
      if (lstat (filename, &sb))
	return;
      ent->st_ino = sb.st_ino;
      ent->st_dev = sb.st_dev;
    }

  {
    struct F_triple *ent_from_table = hash_insert (ht, ent);
    if (ent_from_table == NULL)
      {
	/* Insertion failed due to lack of memory.  */
	xalloc_die ();
      }

    if (ent_from_table != ent)
      {
	/* There was alread a matching entry in the table, so ENT was
	   not inserted.  Free it.  */
	triple_free (ent);
      }
  }
}

/* Copy the file SRC_PATH to the file DST_PATH.  The files may be of
   any type.  NEW_DST should be nonzero if the file DST_PATH cannot
   exist because its parent directory was just created; NEW_DST should
   be zero if DST_PATH might already exist.  DEVICE is the device
   number of the parent directory, or 0 if the parent of this file is
   not known.  ANCESTORS points to a linked, null terminated list of
   devices and inodes of parent directories of SRC_PATH.  COMMAND_LINE_ARG
   is nonzero iff SRC_PATH was specified on the command line.
   Set *COPY_INTO_SELF to nonzero if SRC_PATH is a parent of (or the
   same as) DST_PATH;  otherwise, set it to zero.
   Return 0 if successful, 1 if an error occurs. */

static int
copy_internal (const char *src_path, const char *dst_path,
	       int new_dst,
	       dev_t device,
	       struct dir_list *ancestors,
	       const struct cp_options *x,
	       int command_line_arg,
	       int *copy_into_self,
	       int *rename_succeeded)
{
  struct stat src_sb;
  struct stat dst_sb;
  mode_t src_mode;
  mode_t src_type;
  char *earlier_file = NULL;
  char *dst_backup = NULL;
  int backup_succeeded = 0;
  int delayed_fail;
  int copied_as_regular = 0;
  int ran_chown = 0;
  int preserve_metadata;

  if (x->move_mode && rename_succeeded)
    *rename_succeeded = 0;

  *copy_into_self = 0;
  if ((*(x->xstat)) (src_path, &src_sb))
    {
      error (0, errno, _("cannot stat %s"), quote (src_path));
      return 1;
    }

  src_type = src_sb.st_mode;

  src_mode = src_sb.st_mode;

  if (S_ISDIR (src_type) && !x->recursive)
    {
      error (0, 0, _("omitting directory %s"), quote (src_path));
      return 1;
    }

  /* Detect the case in which the same source file appears more than
     once on the command line and no backup option has been selected.
     If so, simply warn and don't copy it the second time.
     This check is enabled only if x->src_info is non-NULL.  */
  if (command_line_arg)
    {
      if ( ! S_ISDIR (src_sb.st_mode)
	   && x->backup_type == none
	   && seen_file (x->src_info, src_path, &src_sb))
	{
	  error (0, 0, _("warning: source file %s specified more than once"),
		 quote (src_path));
	  return 0;
	}

      record_file (x->src_info, src_path, &src_sb);
    }

  if (!new_dst)
    {
      if ((*(x->xstat)) (dst_path, &dst_sb))
	{
	  if (errno != ENOENT)
	    {
	      error (0, errno, _("cannot stat %s"), quote (dst_path));
	      return 1;
	    }
	  else
	    {
	      new_dst = 1;
	    }
	}
      else
	{
	  int return_now;
	  int ok = same_file_ok (src_path, &src_sb, dst_path, &dst_sb,
				 x, &return_now);
	  if (return_now)
	    return 0;

	  if (! ok)
	    {
	      error (0, 0, _("%s and %s are the same file"),
		     quote_n (0, src_path), quote_n (1, dst_path));
	      return 1;
	    }

	  if (!S_ISDIR (dst_sb.st_mode))
	    {
	      if (S_ISDIR (src_type))
		{
		  error (0, 0,
		     _("cannot overwrite non-directory %s with directory %s"),
			 quote_n (0, dst_path), quote_n (1, src_path));
		  return 1;
		}

	      /* Don't let the user destroy their data, even if they try hard:
		 This mv command must fail (likewise for cp):
		   rm -rf a b c; mkdir a b c; touch a/f b/f; mv a/f b/f c
		 Otherwise, the contents of b/f would be lost.
		 In the case of `cp', b/f would be lost if the user simulated
		 a move using cp and rm.
		 Note that it works fine if you use --backup=numbered.  */
	      if (command_line_arg
		  && x->backup_type != numbered
		  && seen_file (x->dest_info, dst_path, &dst_sb))
		{
		  error (0, 0,
			 _("will not overwrite just-created %s with %s"),
			 quote_n (0, dst_path), quote_n (1, src_path));
		  return 1;
		}
	    }

	  if (!S_ISDIR (src_type))
	    {
	      if (S_ISDIR (dst_sb.st_mode))
		{
		  error (0, 0,
		       _("cannot overwrite directory %s with non-directory"),
			 quote (dst_path));
		  return 1;
		}

	      if (x->update && MTIME_CMP (src_sb, dst_sb) <= 0)
		{
		  /* We're using --update and the source file is older
		     than the destination file, so there is no need to
		     copy or move.  */
		  /* Pretend the rename succeeded, so the caller (mv)
		     doesn't end up removing the source file.  */
		  if (rename_succeeded)
		    *rename_succeeded = 1;
		  return 0;
		}
	    }

	  /* When there is an existing destination file, we may end up
	     returning early, and hence not copying/moving the file.
	     This may be due to an interactive `negative' reply to the
	     prompt about the existing file.  It may also be due to the
	     use of the --reply=no option.  */
	  if (!S_ISDIR (src_type))
	    {
	      /* cp and mv treat -i and -f differently.  */
	      if (x->move_mode)
		{
		  if ((x->interactive == I_ALWAYS_NO
		       && UNWRITABLE (dst_path, dst_sb.st_mode))
		      || ((x->interactive == I_ASK_USER
			   || (x->interactive == I_UNSPECIFIED
			       && x->stdin_tty
			       && UNWRITABLE (dst_path, dst_sb.st_mode)))
			  && (overwrite_prompt (dst_path, &dst_sb), 1)
			  && ! yesno ()))
		    {
		      /* Pretend the rename succeeded, so the caller (mv)
			 doesn't end up removing the source file.  */
		      if (rename_succeeded)
			*rename_succeeded = 1;
		      return 0;
		    }
		}
	      else
		{
		  if (x->interactive == I_ALWAYS_NO
		      || (x->interactive == I_ASK_USER
			  && (overwrite_prompt (dst_path, &dst_sb), 1)
			  && ! yesno ()))
		    {
		      return 0;
		    }
		}
	    }

	  if (x->move_mode)
	    {
	      /* In move_mode, DEST may not be an existing directory.  */
	      if (S_ISDIR (dst_sb.st_mode))
		{
		  error (0, 0, _("cannot overwrite directory %s"),
			 quote (dst_path));
		  return 1;
		}

	      /* Don't allow user to move a directory onto a non-directory.  */
	      if (S_ISDIR (src_sb.st_mode) && !S_ISDIR (dst_sb.st_mode))
		{
		  error (0, 0,
		       _("cannot move directory onto non-directory: %s -> %s"),
			 quote_n (0, src_path), quote_n (0, dst_path));
		  return 1;
		}
	    }

	  if (x->backup_type != none && !S_ISDIR (dst_sb.st_mode))
	    {
	      char *tmp_backup = find_backup_file_name (dst_path,
							x->backup_type);
	      if (tmp_backup == NULL)
		xalloc_die ();

	      /* Detect (and fail) when creating the backup file would
		 destroy the source file.  Before, running the commands
		 cd /tmp; rm -f a a~; : > a; echo A > a~; cp --b=simple a~ a
		 would leave two zero-length files: a and a~.  */
	      /* FIXME: but simply change e.g., the final a~ to `./a~'
		 and the source will still be destroyed.  */
	      if (STREQ (tmp_backup, src_path))
		{
		  const char *fmt;
		  fmt = (x->move_mode
		 ? _("backing up %s would destroy source;  %s not moved")
		 : _("backing up %s would destroy source;  %s not copied"));
		  error (0, 0, fmt,
			 quote_n (0, dst_path),
			 quote_n (1, src_path));
		  free (tmp_backup);
		  return 1;
		}

	      dst_backup = (char *) alloca (strlen (tmp_backup) + 1);
	      strcpy (dst_backup, tmp_backup);
	      free (tmp_backup);
	      if (rename (dst_path, dst_backup))
		{
		  if (errno != ENOENT)
		    {
		      error (0, errno, _("cannot backup %s"), quote (dst_path));
		      return 1;
		    }
		  else
		    {
		      dst_backup = NULL;
		    }
		}
	      else
		{
		  backup_succeeded = 1;
		}
	      new_dst = 1;
	    }
	  else if (! S_ISDIR (dst_sb.st_mode)
		   && (x->unlink_dest_before_opening
		       || (x->xstat == lstat
			   && ! S_ISREG (src_sb.st_mode))))
	    {
	      if (unlink (dst_path) && errno != ENOENT)
		{
		  error (0, errno, _("cannot remove %s"), quote (dst_path));
		  return 1;
		}
	      new_dst = 1;
	    }
	}
    }

  /* If the source is a directory, we don't always create the destination
     directory.  So --verbose should not announce anything until we're
     sure we'll create a directory. */
  if (x->verbose && !S_ISDIR (src_type))
    {
      printf ("%s -> %s", quote_n (0, src_path), quote_n (1, dst_path));
      if (backup_succeeded)
	printf (_(" (backup: %s)"), quote (dst_backup));
      putchar ('\n');
    }

  /* Associate the destination path with the source device and inode
     so that if we encounter a matching dev/ino pair in the source tree
     we can arrange to create a hard link between the corresponding names
     in the destination tree.

     Sometimes, when preserving links, we have to record dev/ino even
     though st_nlink == 1:
     - when using -H and processing a command line argument;
	that command line argument could be a symlink pointing to another
	command line argument.  With `cp -H --preserve=link', we hard-link
	those two destination files.
     - likewise for -L except that it applies to all files, not just
	command line arguments.

     Also record directory dev/ino when using --recursive.  We'll use that
     info to detect this problem: cp -R dir dir.  FIXME-maybe: ideally,
     directory info would be recorded in a separate hash table, since
     such entries are useful only while a single command line hierarchy
     is being copied -- so that separate table could be cleared between
     command line args.  Using the same hash table to preserve hard
     links means that it may not be cleared.  */

  if ((x->preserve_links
       && (1 < src_sb.st_nlink
	   || (command_line_arg
	       && x->dereference == DEREF_COMMAND_LINE_ARGUMENTS)
	   || x->dereference == DEREF_ALWAYS))
      || (x->recursive && S_ISDIR (src_type)))
    {
      earlier_file = remember_copied (dst_path, src_sb.st_ino, src_sb.st_dev);
    }

  /* Did we copy this inode somewhere else (in this command line argument)
     and therefore this is a second hard link to the inode?  */

  if (earlier_file)
    {
      /* Avoid damaging the destination filesystem by refusing to preserve
	 hard-linked directories (which are found at least in Netapp snapshot
	 directories).  */
      if (S_ISDIR (src_type))
	{
	  /* If src_path and earlier_file refer to the same directory entry,
	     then warn about copying a directory into itself.  */
	  if (same_name (src_path, earlier_file))
	    {
	      error (0, 0, _("cannot copy a directory, %s, into itself, %s"),
		     quote_n (0, top_level_src_path),
		     quote_n (1, top_level_dst_path));
	      *copy_into_self = 1;
	    }
	  else
	    {
	      error (0, 0, _("will not create hard link %s to directory %s"),
		     quote_n (0, dst_path), quote_n (1, earlier_file));
	    }

	  goto un_backup;
	}

      {
	int link_failed;

	link_failed = link (earlier_file, dst_path);

	/* If the link failed because of an existing destination,
	   remove that file and then call link again.  */
	if (link_failed && errno == EEXIST)
	  {
	    if (unlink (dst_path))
	      {
		error (0, errno, _("cannot remove %s"), quote (dst_path));
		goto un_backup;
	      }
	    link_failed = link (earlier_file, dst_path);
	  }

	if (link_failed)
	  {
	    error (0, errno, _("cannot create hard link %s to %s"),
		   quote_n (0, dst_path), quote_n (1, earlier_file));
	    goto un_backup;
	  }

	return 0;
      }
    }

  if (x->move_mode)
    {
      if (rename (src_path, dst_path) == 0)
	{
	  if (x->verbose && S_ISDIR (src_type))
	    printf ("%s -> %s\n", quote_n (0, src_path), quote_n (1, dst_path));
	  if (rename_succeeded)
	    *rename_succeeded = 1;

	  if (command_line_arg)
	    {
	      /* Record destination dev/ino/filename, so that if we are asked
		 to overwrite that file again, we can detect it and fail.  */
	      /* It's fine to use the _source_ stat buffer (src_sb) to get the
	         _destination_ dev/ino, since the rename above can't have
		 changed those, and `mv' always uses lstat.
		 We could limit it further by operating
		 only on non-directories.  */
	      record_file (x->dest_info, dst_path, &src_sb);
	    }

	  return 0;
	}

      /* FIXME: someday, consider what to do when moving a directory into
	 itself but when source and destination are on different devices.  */

      /* This happens when attempting to rename a directory to a
	 subdirectory of itself.  */
      if (errno == EINVAL

	  /* When src_path is on an NFS file system, some types of
	     clients, e.g., SunOS4.1.4 and IRIX-5.3, set errno to EIO
	     instead.  Testing for this here risks misinterpreting a real
	     I/O error as an attempt to move a directory into itself, so
	     FIXME: consider not doing this.  */
	  || errno == EIO

	  /* And with SunOS-4.1.4 client and OpenBSD-2.3 server,
	     we get ENOTEMPTY.  */
	  || errno == ENOTEMPTY)
	{
	  /* FIXME: this is a little fragile in that it relies on rename(2)
	     failing with a specific errno value.  Expect problems on
	     non-POSIX systems.  */
	  error (0, 0, _("cannot move %s to a subdirectory of itself, %s"),
		 quote_n (0, top_level_src_path),
		 quote_n (1, top_level_dst_path));

	  /* Note that there is no need to call forget_created here,
	     (compare with the other calls in this file) since the
	     destination directory didn't exist before.  */

	  *copy_into_self = 1;
	  /* FIXME-cleanup: Don't return zero here; adjust mv.c accordingly.
	     The only caller that uses this code (mv.c) ends up setting its
	     exit status to nonzero when copy_into_self is nonzero.  */
	  return 0;
	}

      /* WARNING: there probably exist systems for which an inter-device
	 rename fails with a value of errno not handled here.
	 If/as those are reported, add them to the condition below.
	 If this happens to you, please do the following and send the output
	 to the bug-reporting address (e.g., in the output of cp --help):
	   touch k; perl -e 'rename "k","/tmp/k" or print "$!(",$!+0,")\n"'
	 where your current directory is on one partion and /tmp is the other.
	 Also, please try to find the E* errno macro name corresponding to
	 the diagnostic and parenthesized integer, and include that in your
	 e-mail.  One way to do that is to run a command like this
	   find /usr/include/. -type f \
	     | xargs grep 'define.*\<E[A-Z]*\>.*\<18\>' /dev/null
	 where you'd replace `18' with the integer in parentheses that
	 was output from the perl one-liner above.
	 If necessary, of course, change `/tmp' to some other directory.  */
      if (errno != EXDEV)
	{
	  /* There are many ways this can happen due to a race condition.
	     When something happens between the initial xstat and the
	     subsequent rename, we can get many different types of errors.
	     For example, if the destination is initially a non-directory
	     or non-existent, but it is created as a directory, the rename
	     fails.  If two `mv' commands try to rename the same file at
	     about the same time, one will succeed and the other will fail.
	     If the permissions on the directory containing the source or
	     destination file are made too restrictive, the rename will
	     fail.  Etc.  */
	  error (0, errno,
		 _("cannot move %s to %s"),
		 quote_n (0, src_path), quote_n (1, dst_path));
	  forget_created (src_sb.st_ino, src_sb.st_dev);
	  return 1;
	}

      /* The rename attempt has failed.  Remove any existing destination
	 file so that a cross-device `mv' acts as if it were really using
	 the rename syscall.  */
      if (unlink (dst_path) && errno != ENOENT)
	{
	  error (0, errno,
	     _("inter-device move failed: %s to %s; unable to remove target"),
		 quote_n (0, src_path), quote_n (1, dst_path));
	  forget_created (src_sb.st_ino, src_sb.st_dev);
	  return 1;
	}

      new_dst = 1;
    }

  delayed_fail = 0;

  /* In certain modes (cp's --symbolic-link), and for certain file types
     (symlinks and hard links) it doesn't make sense to preserve metadata,
     or it's possible to preserve only some of it.
     In such cases, set this variable to zero.  */
  preserve_metadata = 1;

  if (S_ISDIR (src_type))
    {
      struct dir_list *dir;

      /* If this directory has been copied before during the
         recursion, there is a symbolic link to an ancestor
         directory of the symbolic link.  It is impossible to
         continue to copy this, unless we've got an infinite disk.  */

      if (is_ancestor (&src_sb, ancestors))
	{
	  error (0, 0, _("cannot copy cyclic symbolic link %s"),
		 quote (src_path));
	  goto un_backup;
	}

      /* Insert the current directory in the list of parents.  */

      dir = (struct dir_list *) alloca (sizeof (struct dir_list));
      dir->parent = ancestors;
      dir->ino = src_sb.st_ino;
      dir->dev = src_sb.st_dev;

      if (new_dst || !S_ISDIR (dst_sb.st_mode))
	{
	  /* Create the new directory writable and searchable, so
             we can create new entries in it.  */

	  if (mkdir (dst_path, (src_mode & x->umask_kill) | S_IRWXU))
	    {
	      error (0, errno, _("cannot create directory %s"),
		     quote (dst_path));
	      goto un_backup;
	    }

	  /* Insert the created directory's inode and device
             numbers into the search structure, so that we can
             avoid copying it again.  */

	  if (remember_created (dst_path))
	    goto un_backup;

	  if (x->verbose)
	    printf ("%s -> %s\n", quote_n (0, src_path), quote_n (1, dst_path));
	}

      /* Are we crossing a file system boundary?  */
      if (x->one_file_system && device != 0 && device != src_sb.st_dev)
	return 0;

      /* Copy the contents of the directory.  */

      if (copy_dir (src_path, dst_path, new_dst, &src_sb, dir, x,
		    copy_into_self))
	{
	  /* Don't just return here -- otherwise, the failure to read a
	     single file in a source directory would cause the containing
	     destination directory not to have owner/perms set properly.  */
	  delayed_fail = 1;
	}
    }
#ifdef S_ISLNK
  else if (x->symbolic_link)
    {
      preserve_metadata = 0;

      if (*src_path != '/')
	{
	  /* Check that DST_PATH denotes a file in the current directory.  */
	  struct stat dot_sb;
	  struct stat dst_parent_sb;
	  char *dst_parent;
	  int in_current_dir;

	  dst_parent = dir_name (dst_path);

	  in_current_dir = (STREQ (".", dst_parent)
			    /* If either stat call fails, it's ok not to report
			       the failure and say dst_path is in the current
			       directory.  Other things will fail later.  */
			    || stat (".", &dot_sb)
			    || stat (dst_parent, &dst_parent_sb)
			    || SAME_INODE (dot_sb, dst_parent_sb));
	  free (dst_parent);

	  if (! in_current_dir)
	    {
	      error (0, 0,
	   _("%s: can make relative symbolic links only in current directory"),
		     quote (dst_path));
	      goto un_backup;
	    }
	}
      if (symlink (src_path, dst_path))
	{
	  error (0, errno, _("cannot create symbolic link %s to %s"),
		 quote_n (0, dst_path), quote_n (1, src_path));
	  goto un_backup;
	}
    }
#endif
  else if (x->hard_link)
    {
      preserve_metadata = 0;
      if (link (src_path, dst_path))
	{
	  error (0, errno, _("cannot create link %s"), quote (dst_path));
	  goto un_backup;
	}
    }
  else if (S_ISREG (src_type)
	   || (x->copy_as_regular && !S_ISDIR (src_type)
#ifdef S_ISLNK
	       && !S_ISLNK (src_type)
#endif
	       ))
    {
      copied_as_regular = 1;
      /* POSIX says the permission bits of the source file must be
	 used as the 3rd argument in the open call, but that's not consistent
	 with historical practice.  */
      if (copy_reg (src_path, dst_path, x,
		    get_dest_mode (x, src_mode), &new_dst, &src_sb))
	goto un_backup;
    }
  else
#ifdef S_ISFIFO
  if (S_ISFIFO (src_type))
    {
      if (mkfifo (dst_path, get_dest_mode (x, src_mode)))
	{
	  error (0, errno, _("cannot create fifo %s"), quote (dst_path));
	  goto un_backup;
	}
    }
  else
#endif
    if (S_ISBLK (src_type) || S_ISCHR (src_type)
#ifdef S_ISSOCK
	|| S_ISSOCK (src_type)
#endif
	)
    {
      if (mknod (dst_path, get_dest_mode (x, src_mode), src_sb.st_rdev))
	{
	  error (0, errno, _("cannot create special file %s"),
		 quote (dst_path));
	  goto un_backup;
	}
    }
  else
#ifdef S_ISLNK
  if (S_ISLNK (src_type))
    {
      char *src_link_val = xreadlink (src_path);
      if (src_link_val == NULL)
	{
	  error (0, errno, _("cannot read symbolic link %s"), quote (src_path));
	  goto un_backup;
	}

      if (!symlink (src_link_val, dst_path))
	free (src_link_val);
      else
	{
	  int saved_errno = errno;
	  int same_link = 0;
	  if (x->update && !new_dst && S_ISLNK (dst_sb.st_mode))
	    {
	      /* See if the destination is already the desired symlink.  */
	      size_t src_link_len = strlen (src_link_val);
	      char *dest_link_val = (char *) alloca (src_link_len + 1);
	      int dest_link_len = readlink (dst_path, dest_link_val,
					    src_link_len + 1);
	      if ((size_t) dest_link_len == src_link_len
		  && strncmp (dest_link_val, src_link_val, src_link_len) == 0)
		same_link = 1;
	    }
	  free (src_link_val);

	  if (! same_link)
	    {
	      error (0, saved_errno, _("cannot create symbolic link %s"),
		     quote (dst_path));
	      goto un_backup;
	    }
	}

      /* There's no need to preserve timestamps or permissions.  */
      preserve_metadata = 0;

      if (x->preserve_ownership)
	{
	  /* Preserve the owner and group of the just-`copied'
	     symbolic link, if possible.  */
# if HAVE_LCHOWN
	  if (DO_CHOWN (lchown, dst_path, src_sb.st_uid, src_sb.st_gid))
	    {
	      error (0, errno, _("failed to preserve ownership for %s"),
		     dst_path);
	      goto un_backup;
	    }
# else
	  /* Can't preserve ownership of symlinks.
	     FIXME: maybe give a warning or even error for symlinks
	     in directories with the sticky bit set -- there, not
	     preserving owner/group is a potential security problem.  */
# endif
	}
    }
  else
#endif
    {
      error (0, 0, _("%s has unknown file type"), quote (src_path));
      goto un_backup;
    }

  if (command_line_arg)
    record_file (x->dest_info, dst_path, NULL);

  if ( ! preserve_metadata)
    return 0;

  /* POSIX says that `cp -p' must restore the following:
     - permission bits
     - setuid, setgid bits
     - owner and group
     If it fails to restore any of those, we may give a warning but
     the destination must not be removed.
     FIXME: implement the above. */

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
	  error (0, errno, _("preserving times for %s"), quote (dst_path));
	  if (x->require_preserve)
	    return 1;
	}
    }

  /* Avoid calling chown if we know it's not necessary.  */
  if (x->preserve_ownership
      && (new_dst || !SAME_OWNER_AND_GROUP (src_sb, dst_sb)))
    {
      ran_chown = 1;
      if (DO_CHOWN (chown, dst_path, src_sb.st_uid, src_sb.st_gid))
	{
	  error (0, errno, _("failed to preserve ownership for %s"),
		 quote (dst_path));
	  if (x->require_preserve)
	    return 1;
	}
    }

#if HAVE_STRUCT_STAT_ST_AUTHOR
  /* Preserve the st_author field.  */
  {
    file_t file = file_name_lookup (dst_path, 0, 0);
    if (file_chauthor (file, src_sb.st_author))
      error (0, errno, _("failed to preserve authorship for %s"),
	     quote (dst_path));
    mach_port_deallocate (mach_task_self (), file);
  }
#endif

  /* Permissions of newly-created regular files were set upon `open' in
     copy_reg.  But don't return early if there were any special bits and
     we had to run chown, because the chown must have reset those bits.  */
  if ((new_dst && copied_as_regular)
      && !(ran_chown && (src_mode & ~S_IRWXUGO)))
    return delayed_fail;

  if ((x->preserve_mode || new_dst)
      && (x->copy_as_regular || S_ISREG (src_type) || S_ISDIR (src_type)))
    {
      if (chmod (dst_path, get_dest_mode (x, src_mode)))
	{
	  error (0, errno, _("setting permissions for %s"), quote (dst_path));
	  if (x->set_mode || x->require_preserve)
	    return 1;
	}
    }

  return delayed_fail;

un_backup:

  /* We have failed to create the destination file.
     If we've just added a dev/ino entry via the remember_copied
     call above (i.e., unless we've just failed to create a hard link),
     remove the entry associating the source dev/ino with the
     destination file name, so we don't try to `preserve' a link
     to a file we didn't create.  */
  if (earlier_file == NULL)
    forget_created (src_sb.st_ino, src_sb.st_dev);

  if (dst_backup)
    {
      if (rename (dst_backup, dst_path))
	error (0, errno, _("cannot un-backup %s"), quote (dst_path));
      else
	{
	  if (x->verbose)
	    printf (_("%s -> %s (unbackup)\n"),
		    quote_n (0, dst_backup), quote_n (1, dst_path));
	}
    }
  return 1;
}

static int
valid_options (const struct cp_options *co)
{
  assert (co != NULL);

  assert (VALID_BACKUP_TYPE (co->backup_type));

  /* FIXME: for some reason this assertion always fails,
     at least on Solaris2.5.1.  Just disable it for now.  */
  /* assert (co->xstat == lstat || co->xstat == stat); */

  /* Make sure xstat and dereference are consistent.  */
  /* FIXME */

  assert (VALID_SPARSE_MODE (co->sparse_mode));

  return 1;
}

/* Copy the file SRC_PATH to the file DST_PATH.  The files may be of
   any type.  NONEXISTENT_DST should be nonzero if the file DST_PATH
   is known not to exist (e.g., because its parent directory was just
   created);  NONEXISTENT_DST should be zero if DST_PATH might already
   exist.  OPTIONS is ... FIXME-describe
   Set *COPY_INTO_SELF to nonzero if SRC_PATH is a parent of (or the
   same as) DST_PATH;  otherwise, set it to zero.
   Return 0 if successful, 1 if an error occurs. */

int
copy (const char *src_path, const char *dst_path,
      int nonexistent_dst, const struct cp_options *options,
      int *copy_into_self, int *rename_succeeded)
{
  assert (valid_options (options));

  /* Record the file names: they're used in case of error, when copying
     a directory into itself.  I don't like to make these tools do *any*
     extra work in the common case when that work is solely to handle
     exceptional cases, but in this case, I don't see a way to derive the
     top level source and destination directory names where they're used.
     An alternative is to use COPY_INTO_SELF and print the diagnostic
     from every caller -- but I don't want to do that.  */
  top_level_src_path = src_path;
  top_level_dst_path = dst_path;

  return copy_internal (src_path, dst_path, nonexistent_dst, 0, NULL,
			options, 1, copy_into_self, rename_succeeded);
}
