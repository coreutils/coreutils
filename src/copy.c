/* copy.c -- core functions for copying files and directories
   Copyright (C) 89, 90, 91, 1995-2001 Free Software Foundation.

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

#include "system.h"
#include "error.h"
#include "backupfile.h"
#include "savedir.h"
#include "copy.h"
#include "cp-hash.h"
#include "dirname.h"
#include "path-concat.h"
#include "quote.h"
#include "same.h"

#define DO_CHOWN(Chown, File, New_uid, New_gid)				\
  (Chown (File, New_uid, New_gid)					\
   /* If non-root uses -p, it's ok if we can't preserve ownership.	\
      But root probably wants to know, e.g. if NFS disallows it,	\
      or if the target system doesn't support file ownership.  */	\
   && ((errno != EPERM && errno != EINVAL) || x->myeuid == 0))

#define SAME_OWNER(A, B) ((A).st_uid == (B).st_uid)
#define SAME_GROUP(A, B) ((A).st_gid == (B).st_gid)
#define SAME_OWNER_AND_GROUP(A, B) (SAME_OWNER (A, B) && SAME_GROUP (A, B))

struct dir_list
{
  struct dir_list *parent;
  ino_t ino;
  dev_t dev;
};

int full_write ();
int euidaccess ();
int yesno ();

static int copy_internal PARAMS ((const char *src_path, const char *dst_path,
				  int new_dst, dev_t device,
				  struct dir_list *ancestors,
				  const struct cp_options *x,
				  int move_mode,
				  int *copy_into_self,
				  int *rename_succeeded));

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

  /* Honor the umask for `cp', but not for `mv' or `cp -p'.  */
  if (!option->move_mode && !option->preserve_chmod_bits)
    mode &= option->umask_kill;

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

  name_space = savedir (src_path_in, src_sb->st_size);
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

      /* Free the memory for `src_path'.  The memory for `dst_path'
	 cannot be deallocated, since it is used to create multiple
	 hard links.  */

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
   *NEW_DST is as in copy_internal.  */

static int
copy_reg (const char *src_path, const char *dst_path,
	  const struct cp_options *x, mode_t dst_mode, int *new_dst)
{
  char *buf;
  int buf_size;
  int dest_desc;
  int source_desc;
  struct stat sb;
  char *cp;
  int *ip;
  int return_val = 0;
  off_t n_read_total = 0;
  int last_write_made_hole = 0;
  int make_holes = (x->sparse_mode == SPARSE_ALWAYS);

  source_desc = open (src_path, O_RDONLY);
  if (source_desc < 0)
    {
      /* If SRC_PATH doesn't exist, then chances are good that the
	 user did something like this `cp --backup foo foo': and foo
	 existed to start with, but copy_internal renamed DST_PATH
	 with the backup suffix, thus also renaming SRC_PATH.  */
      if (errno == ENOENT)
	error (0, 0, _("%s and %s are the same file"),
	       quote_n (0, src_path), quote_n (1, dst_path));
      else
	error (0, errno, _("cannot open %s for reading"), quote (src_path));

      return -1;
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

  /* Find out the optimal buffer size.  */

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
      int n_read = read (source_desc, buf, buf_size);
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
	  if (full_write (dest_desc, buf, n_read) < 0)
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
      if (full_write (dest_desc, "", 1) < 0
	  || ftruncate (dest_desc, n_read_total) < 0)
#else
      /* Seek backwards one character and write a null.  */
      if (lseek (dest_desc, (off_t) -1, SEEK_CUR) < 0L
	  || full_write (dest_desc, "", 1) < 0)
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
	 then it's ok.  */
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

  /* The backup code ensures there's a copy, so it's ok to remove
     any destination file.  But there's one exception: when both
     source and destination are the same directory entry.  In that
     case, moving the destination file aside (in making the backup)
     would also rename the source file and result in an error.  */
  if (x->backup_type != none)
    {
      if (!same_link)
	return 1;

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

/* Copy the file SRC_PATH to the file DST_PATH.  The files may be of
   any type.  NEW_DST should be nonzero if the file DST_PATH cannot
   exist because its parent directory was just created; NEW_DST should
   be zero if DST_PATH might already exist.  DEVICE is the device
   number of the parent directory, or 0 if the parent of this file is
   not known.  ANCESTORS points to a linked, null terminated list of
   devices and inodes of parent directories of SRC_PATH.
   Set *COPY_INTO_SELF to nonzero if SRC_PATH is a parent of (or the
   same as) DST_PATH;  otherwise, set it to zero.
   Return 0 if successful, 1 if an error occurs. */

static int
copy_internal (const char *src_path, const char *dst_path,
	       int new_dst,
	       dev_t device,
	       struct dir_list *ancestors,
	       const struct cp_options *x,
	       int move_mode,
	       int *copy_into_self,
	       int *rename_succeeded)
{
  struct stat src_sb;
  struct stat dst_sb;
  mode_t src_mode;
  mode_t src_type;
  char *earlier_file;
  char *dst_backup = NULL;
  int backup_succeeded = 0;
  int rename_errno;
  int delayed_fail;
  int copied_as_regular = 0;
  int ran_chown = 0;

  if (move_mode && rename_succeeded)
    *rename_succeeded = 0;

  *copy_into_self = 0;
  if ((*(x->xstat)) (src_path, &src_sb))
    {
      error (0, errno, _("cannot stat %s"), quote (src_path));
      return 1;
    }

  /* We wouldn't insert a node unless nlink > 1, except that we need to
     find created files so as to not copy infinitely if a directory is
     copied into itself.  */

  /* Associate the destination path with the source device and inode
     so that if we encounter a matching dev/ino pair in the source tree
     we can arrange to create a hard link between the corresponding names
     in the destination tree.  */
  earlier_file = remember_copied (dst_path, src_sb.st_ino, src_sb.st_dev);

  src_mode = src_sb.st_mode;
  src_type = src_sb.st_mode;

  if (S_ISDIR (src_type) && !x->recursive)
    {
      error (0, 0, _("omitting directory %s"), quote (src_path));
      return 1;
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

	  if (S_ISDIR (src_type) && !S_ISDIR (dst_sb.st_mode))
	    {
	      error (0, 0,
		     _("cannot overwrite non-directory %s with directory %s"),
		     quote_n (0, dst_path), quote_n (1, src_path));
	      return 1;
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
		return 0;
	    }

	  if (!S_ISDIR (src_type) && x->interactive)
	    {
	      if (euidaccess (dst_path, W_OK) != 0)
		{
		  fprintf (stderr,
			   _("%s: overwrite %s, overriding mode %04lo? "),
			   program_name, quote (dst_path),
			   (unsigned long) (dst_sb.st_mode & CHMOD_MODE_BITS));
		}
	      else
		{
		  fprintf (stderr, _("%s: overwrite %s? "),
			   program_name, quote (dst_path));
		}
	      if (!yesno ())
		return (move_mode ? 1 : 0);
	    }

	  if (move_mode)
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
		 cd /tmp; rm -f a a~; : > a; echo A > a~; cp -b -V simple a~ a
		 would leave two zero-length files: a and a~.  */
	      if (STREQ (tmp_backup, src_path))
		{
		  const char *fmt;
		  fmt = (move_mode
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

  /* Did we copy this inode somewhere else (in this command line argument)
     and therefore this is a second hard link to the inode?  */

  if (x->dereference == DEREF_NEVER && src_sb.st_nlink > 1 && earlier_file)
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

      if (link (earlier_file, dst_path))
	{
	  error (0, errno, _("cannot create hard link %s to %s"),
		 quote_n (0, dst_path), quote_n (1, earlier_file));
	  goto un_backup;
	}

      return 0;
    }

  if (move_mode)
    {
      if (rename (src_path, dst_path) == 0)
	{
	  if (x->verbose && S_ISDIR (src_type))
	    printf ("%s -> %s\n", quote_n (0, src_path), quote_n (1, dst_path));
	  if (rename_succeeded)
	    *rename_succeeded = 1;
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
	  *copy_into_self = 1;
	  /* FIXME-cleanup: Don't return zero here; adjust mv.c accordingly.
	     The only caller that uses this code (mv.c) ends up setting its
	     exit status to nonzero when copy_into_self is nonzero.  */
	  return 0;
	}

      /* Ignore other types of failure (e.g. EXDEV), since the following
	 code will try to perform a copy, then remove.  */

      /* Save this value of errno to use in case the unlink fails.  */
      rename_errno = errno;

      /* The rename attempt has failed.  Remove any existing destination
	 file so that a cross-device `mv' acts as if it were really using
	 the rename syscall.  */
      if (unlink (dst_path) && errno != ENOENT)
	{
	  /* Use the value of errno from the failed rename.  */
	  error (0, rename_errno, _("cannot move %s to %s"),
		 quote_n (0, src_path), quote_n (1, dst_path));
	  return 1;
	}

      new_dst = 1;
    }

  delayed_fail = 0;
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
      if (*src_path != '/')
	{
	  /* Check that DST_PATH denotes a file in the current directory.  */
	  struct stat dot_sb;
	  struct stat dst_parent_sb;
	  char *dst_parent;
	  int in_current_dir;

	  dst_parent = dir_name (dst_path);
	  if (dst_parent == NULL)
	    xalloc_die ();

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

      return 0;
    }
#endif
  else if (x->hard_link)
    {
      if (link (src_path, dst_path))
	{
	  error (0, errno, _("cannot create link %s"), quote (dst_path));
	  goto un_backup;
	}
      return 0;
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
		    get_dest_mode (x, src_mode), &new_dst))
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
      char *link_val;
      int link_size;

      link_val = (char *) alloca (PATH_MAX + 2);
      link_size = readlink (src_path, link_val, PATH_MAX + 1);
      if (link_size < 0)
	{
	  error (0, errno, _("cannot read symbolic link %s"), quote (src_path));
	  goto un_backup;
	}
      link_val[link_size] = '\0';

      if (symlink (link_val, dst_path))
	{
	  int saved_errno = errno;
	  int same_link = 0;
	  if (x->update && !new_dst && S_ISLNK (dst_sb.st_mode))
	    {
	      /* See if the destination is already the desired symlink.  */
	      char *dest_link_name = (char *) alloca (PATH_MAX + 2);
	      int dest_link_len = readlink (dst_path, dest_link_name,
					    PATH_MAX + 1);
	      if (dest_link_len > 0)
		{
		  dest_link_name[dest_link_len] = '\0';
		  if (STREQ (dest_link_name, link_val))
		    same_link = 1;
		}
	    }

	  if (! same_link)
	    {
	      error (0, saved_errno, _("cannot create symbolic link %s"),
		     quote (dst_path));
	      goto un_backup;
	    }
	}

      if (x->preserve_owner_and_group)
	{
	  /* Preserve the owner and group of the just-`copied'
	     symbolic link, if possible.  */
# if HAVE_LCHOWN
	  if (DO_CHOWN (lchown, dst_path, src_sb.st_uid, src_sb.st_gid))
	    {
	      error (0, errno, _("preserving ownership for %s"), dst_path);
	      goto un_backup;
	    }
# else
	  /* Can't preserve ownership of symlinks.
	     FIXME: maybe give a warning or even error for symlinks
	     in directories with the sticky bit set -- there, not
	     preserving owner/group is a potential security problem.  */
# endif
	}

      return 0;
    }
  else
#endif
    {
      error (0, 0, _("%s has unknown file type"), quote (src_path));
      goto un_backup;
    }

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
  if (x->preserve_owner_and_group
      && (new_dst || !SAME_OWNER_AND_GROUP (src_sb, dst_sb)))
    {
      ran_chown = 1;
      if (DO_CHOWN (chown, dst_path, src_sb.st_uid, src_sb.st_gid))
	{
	  error (0, errno, _("preserving ownership for %s"), quote (dst_path));
	  if (x->require_preserve)
	    return 1;
	}
    }

  /* Permissions of newly-created regular files were set upon `open' in
     copy_reg.  But don't return early if there were any special bits and
     we had to run chown, because the chown must have reset those bits.  */
  if ((new_dst && copied_as_regular)
      && !(ran_chown && (src_mode & ~S_IRWXUGO)))
    return delayed_fail;

  if ((x->preserve_chmod_bits || new_dst)
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
  /* move_mode is set to the value from the `options' parameter for the
     first copy_internal call.  For all subsequent calls (if any), it must
     be zero.  */
  int move_mode = options->move_mode;

  assert (valid_options (options));

  /* Record the file names: they're used in case of error, when copying
     a directory into itself.  I don't like to make these tools do *any*
     extra work in the common case when that work is solely to handle
     exceptional cases, but in this case, I don't see a way to derive the
     top level source and destination directory names where they're used.
     An alternative is to use COPY_INTO_SELF and print the diagnostic
     from every caller -- but I don't wan't to do that.  */
  top_level_src_path = src_path;
  top_level_dst_path = dst_path;

  return copy_internal (src_path, dst_path, nonexistent_dst, 0, NULL,
			options, move_mode, copy_into_self, rename_succeeded);
}
