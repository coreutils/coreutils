/* copy.c -- core functions for copying files and directories
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
#include "path-concat.h"

#define DO_CHOWN(Chown, File, New_uid, New_gid)				\
  (Chown ((File), (x->myeuid == 0 ? (New_uid) : x->myeuid), (New_gid))	\
   /* If non-root uses -p, it's ok if we can't preserve ownership.	\
      But root probably wants to know, e.g. if NFS disallows it,	\
      or if the target system doesn't support file ownership. */	\
   && ((errno != EPERM && errno != EINVAL) || x->myeuid == 0))

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

/* The invocation name of this program.  */
extern char *program_name;

/* Encapsulate selection of the file mode to be applied to
   new non-directories.  */

static mode_t
new_nondir_mode (const struct cp_options *option, mode_t mode)
{
  /* In some applications (e.g., install), use precisely the
     specified mode.  */
  if (option->set_mode)
    return option->mode;

  /* In others (e.g., cp, mv), apply the user's umask.  */
  return mode & option->umask_kill;
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
  int ret = 0;

  errno = 0;
  name_space = savedir (src_path_in, (unsigned int) src_sb->st_size);
  if (name_space == 0)
    {
      if (errno)
	{
	  error (0, errno, "%s", src_path_in);
	  return -1;
	}
      else
	error (1, 0, _("virtual memory exhausted"));
    }

  namep = name_space;
  while (*namep != '\0')
    {
      int local_copy_into_self;
      char *src_path = path_concat (src_path_in, namep, NULL);
      char *dst_path = path_concat (dst_path_in, namep, NULL);

      if (dst_path == NULL || src_path == NULL)
	error (1, 0, _("virtual memory exhausted"));

      ret |= copy_internal (src_path, dst_path, new_dst, src_sb->st_dev,
			    ancestors, x, 0, &local_copy_into_self, NULL);
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
   Return 0 if successful, -1 if an error occurred.
   FIXME: describe sparse_mode.  */

static int
copy_reg (const char *src_path, const char *dst_path,
	  enum Sparse_type sparse_mode)
{
  char *buf;
  int buf_size;
  int dest_desc;
  int source_desc;
  int n_read;
  struct stat sb;
  char *cp;
  int *ip;
  int return_val = 0;
  off_t n_read_total = 0;
  int last_write_made_hole = 0;
  int make_holes = (sparse_mode == SPARSE_ALWAYS);

  source_desc = open (src_path, O_RDONLY);
  if (source_desc < 0)
    {
      /* If SRC_PATH doesn't exist, then chances are good that the
	 user did something like this `cp --backup foo foo': and foo
	 existed to start with, but copy_internal renamed DST_PATH
	 with the backup suffix, thus also renaming SRC_PATH.  */
      if (errno == ENOENT)
	error (0, 0, _("`%s' and `%s' are the same file"),
	       src_path, dst_path);
      else
	error (0, errno, "%s", src_path);

      return -1;
    }

  /* Create the new regular file with small permissions initially,
     to not create a security hole.  */

  dest_desc = open (dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (dest_desc < 0)
    {
      error (0, errno, _("cannot create regular file `%s'"), dst_path);
      return_val = -1;
      goto ret2;
    }

  /* Find out the optimal buffer size.  */

  if (fstat (dest_desc, &sb))
    {
      error (0, errno, "%s", dst_path);
      return_val = -1;
      goto ret;
    }

  buf_size = ST_BLKSIZE (sb);

#if HAVE_ST_BLOCKS
  if (sparse_mode == SPARSE_AUTO && S_ISREG (sb.st_mode))
    {
      /* Use a heuristic to determine whether SRC_PATH contains any
	 sparse blocks. */

      if (fstat (source_desc, &sb))
	{
	  error (0, errno, "%s", src_path);
	  return_val = -1;
	  goto ret;
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
      n_read = read (source_desc, buf, buf_size);
      if (n_read < 0)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
#endif
	  error (0, errno, "%s", src_path);
	  return_val = -1;
	  goto ret;
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
		  error (0, errno, "%s", dst_path);
		  return_val = -1;
		  goto ret;
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
	      error (0, errno, "%s", dst_path);
	      return_val = -1;
	      goto ret;
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
	  error (0, errno, "%s", dst_path);
	  return_val = -1;
	}
    }

ret:
  if (close (dest_desc) < 0)
    {
      error (0, errno, "%s", dst_path);
      return_val = -1;
    }
ret2:
  if (close (source_desc) < 0)
    {
      error (0, errno, "%s", src_path);
      return_val = -1;
    }

  return return_val;
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
  int src_mode;
  int src_type;
  char *earlier_file;
  char *dst_backup = NULL;
  int fix_mode = 0;

  if (move_mode && rename_succeeded)
    *rename_succeeded = 0;

  *copy_into_self = 0;
  if ((*(x->xstat)) (src_path, &src_sb))
    {
      error (0, errno, "%s", src_path);
      return 1;
    }

  /* Are we crossing a file system boundary?  */
  if (x->one_file_system && device != 0 && device != src_sb.st_dev)
    return 0;

  /* We wouldn't insert a node unless nlink > 1, except that we need to
     find created files so as to not copy infinitely if a directory is
     copied into itself.  */

  earlier_file = remember_copied (dst_path, src_sb.st_ino, src_sb.st_dev);

  /* Did we just create this file?  */

  if (earlier_file == &new_file)
    {
      *copy_into_self = 1;
      return 0;
    }

  src_mode = src_sb.st_mode;
  src_type = src_sb.st_mode;

  if (S_ISDIR (src_type) && !x->recursive)
    {
      error (0, 0, _("%s: omitting directory"), src_path);
      return 1;
    }

  if (!new_dst)
    {
      if ((*(x->xstat)) (dst_path, &dst_sb))
	{
	  if (errno != ENOENT)
	    {
	      error (0, errno, "%s", dst_path);
	      return 1;
	    }
	  else
	    {
	      new_dst = 1;
	    }
	}
      else
	{
	  int same;

	  /* The destination file exists already.  */

	  same = (src_sb.st_ino == dst_sb.st_ino
		  && src_sb.st_dev == dst_sb.st_dev);

#ifdef S_ISLNK
	  /* If we're preserving symlinks (--no-dereference) and either
	     file is a symlink, use stat (not xstat) to see if they refer
	     to the same file.  */
	  if (!same
	      /* If we're making a backup, we'll detect the problem case in
		 copy_reg because SRC_PATH will no longer exist.  Allowing
		 the test to be deferred lets cp do some useful things.  */
	      && x->backup_type == none
	      && !x->dereference
	      && (S_ISLNK (dst_sb.st_mode) || S_ISLNK (src_sb.st_mode)))
	    {
	      struct stat dst2_sb;
	      struct stat src2_sb;
	      if (stat (dst_path, &dst2_sb) == 0
		  && stat (src_path, &src2_sb) == 0
		  && src2_sb.st_ino == dst2_sb.st_ino
		  && src2_sb.st_dev == dst2_sb.st_dev)
		{
		  same = 1;
		}
	    }
#endif

	  if (same)
	    {
	      if (x->hard_link)
		return 0;

	      if (x->backup_type == none)
		{
		  error (0, 0, _("`%s' and `%s' are the same file"),
			 src_path, dst_path);
		  return 1;
		}
	    }

	  if (!S_ISDIR (src_type))
	    {
	      if (S_ISDIR (dst_sb.st_mode))
		{
		  error (0, 0,
		       _("%s: cannot overwrite directory with non-directory"),
			 dst_path);
		  return 1;
		}

	      if (x->update && src_sb.st_mtime <= dst_sb.st_mtime)
		return 0;
	    }

	  if (!S_ISDIR (src_type) && !x->force && x->interactive)
	    {
	      if (euidaccess (dst_path, W_OK) != 0)
		{
		  fprintf (stderr,
			   _("%s: overwrite `%s', overriding mode %04o? "),
			   program_name, dst_path,
			   (unsigned int) (dst_sb.st_mode & 07777));
		}
	      else
		{
		  fprintf (stderr, _("%s: overwrite `%s'? "),
			   program_name, dst_path);
		}
	      if (!yesno ())
		return 0;
	    }

	  /* In move_mode, DEST may not be an existing directory.  */
	  if (move_mode && S_ISDIR (dst_sb.st_mode))
	    {
	      error (0, 0, _("%s: cannot overwrite directory"), dst_path);
	      return 1;
	    }

	  if (x->backup_type != none && !S_ISDIR (dst_sb.st_mode))
	    {
	      char *tmp_backup = find_backup_file_name (dst_path,
							x->backup_type);
	      if (tmp_backup == NULL)
		error (1, 0, _("virtual memory exhausted"));

	      /* Detect (and fail) when creating the backup file would
		 destroy the source file.  Before, running the commands
		 cd /tmp; rm -f a a~; : > a; echo A > a~; cp -b -V simple a~ a
		 would leave two zero-length files: a and a~.  */
	      if (STREQ (tmp_backup, src_path))
		{
		  const char *fmt;
		  fmt = (move_mode
		 ? _("backing up `%s' would destroy source;  `%s' not moved")
		 : _("backing up `%s' would destroy source;  `%s' not copied"));
		  error (0, 0, fmt, dst_path, src_path);
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
		      error (0, errno, _("cannot backup `%s'"), dst_path);
		      return 1;
		    }
		  else
		    dst_backup = NULL;
		}
	      new_dst = 1;
	    }
	  else if (x->force)
	    {
	      if (S_ISDIR (dst_sb.st_mode))
		{
		  /* Temporarily change mode to allow overwriting. */
		  if (euidaccess (dst_path, W_OK | X_OK) != 0)
		    {
		      if (chmod (dst_path, 0700))
			{
			  error (0, errno, "%s", dst_path);
			  return 1;
			}
		      else
			fix_mode = 1;
		    }
		}
	      else
		{
		  if (unlink (dst_path) && errno != ENOENT)
		    {
		      error (0, errno, _("cannot remove old link to `%s'"),
			     dst_path);
		      if (x->failed_unlink_is_fatal)
			return 1;
		    }
		  else
		    {
		      new_dst = 1;
		    }
		}
	    }
	}
    }

  /* If the source is a directory, we don't always create the destination
     directory.  So --verbose should not announce anything until we're
     sure we'll create a directory. */
  if (x->verbose && !S_ISDIR (src_type))
    printf ("%s -> %s\n", src_path, dst_path);

  /* Did we copy this inode somewhere else (in this command line argument)
     and therefore this is a second hard link to the inode?  */

  if (!x->dereference && src_sb.st_nlink > 1 && earlier_file)
    {
      if (link (earlier_file, dst_path))
	{
	  error (0, errno, "%s", dst_path);
	  goto un_backup;
	}
      return 0;
    }

  if (move_mode && rename (src_path, dst_path) == 0)
    {
      if (x->verbose && S_ISDIR (src_type))
	printf ("%s -> %s\n", src_path, dst_path);
      if (rename_succeeded)
	*rename_succeeded = 1;
      return 0;
    }

  if (S_ISDIR (src_type))
    {
      struct dir_list *dir;

      /* If this directory has been copied before during the
         recursion, there is a symbolic link to an ancestor
         directory of the symbolic link.  It is impossible to
         continue to copy this, unless we've got an infinite disk.  */

      if (is_ancestor (&src_sb, ancestors))
	{
	  error (0, 0, _("%s: cannot copy cyclic symbolic link"), src_path);
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

	  if (mkdir (dst_path, (src_mode & x->umask_kill) | 0700))
	    {
	      error (0, errno, _("cannot create directory `%s'"), dst_path);
	      goto un_backup;
	    }

	  /* Insert the created directory's inode and device
             numbers into the search structure, so that we can
             avoid copying it again.  */

	  if (remember_created (dst_path))
	    goto un_backup;

	  if (x->verbose)
	    printf ("%s -> %s\n", src_path, dst_path);
	}

      /* Copy the contents of the directory.  */

      if (copy_dir (src_path, dst_path, new_dst, &src_sb, dir, x,
		    copy_into_self))
	return 1;
    }
#ifdef S_ISLNK
  else if (x->symbolic_link)
    {
      if (*src_path == '/'
	  || (!strncmp (dst_path, "./", 2) && strchr (dst_path + 2, '/') == 0)
	  || strchr (dst_path, '/') == 0)
	{
	  if (symlink (src_path, dst_path))
	    {
	      error (0, errno, "%s", dst_path);
	      goto un_backup;
	    }

	  return 0;
	}
      else
	{
	  error (0, 0,
	   _("%s: can make relative symbolic links only in current directory"),
		 dst_path);
	  goto un_backup;
	}
    }
#endif
  else if (x->hard_link)
    {
      if (link (src_path, dst_path))
	{
	  error (0, errno, _("cannot create link `%s'"), dst_path);
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
      if (copy_reg (src_path, dst_path, x->sparse_mode))
	goto un_backup;
    }
  else
#ifdef S_ISFIFO
  if (S_ISFIFO (src_type))
    {
      if (mkfifo (dst_path, new_nondir_mode (x, src_mode)))
	{
	  error (0, errno, _("cannot create fifo `%s'"), dst_path);
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
      if (mknod (dst_path, new_nondir_mode (x, src_mode), src_sb.st_rdev))
	{
	  error (0, errno, _("cannot create special file `%s'"), dst_path);
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
	  error (0, errno, _("cannot read symbolic link `%s'"), src_path);
	  goto un_backup;
	}
      link_val[link_size] = '\0';

      if (symlink (link_val, dst_path))
	{
	  error (0, errno, _("cannot create symbolic link `%s'"), dst_path);
	  goto un_backup;
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
      error (0, 0, _("%s: unknown file type"), src_path);
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

      utb.actime = src_sb.st_atime;
      utb.modtime = src_sb.st_mtime;

      if (utime (dst_path, &utb))
	{
	  error (0, errno, _("preserving times for %s"), dst_path);
	  if (x->require_preserve)
	    return 1;
	}
    }

  if (x->preserve_owner_and_group)
    {
      if (DO_CHOWN (chown, dst_path, src_sb.st_uid, src_sb.st_gid))
	{
	  error (0, errno, _("preserving ownership for %s"), dst_path);
	  if (x->require_preserve)
	    return 1;
	}
    }

  if (x->set_mode)
    {
      /* This is so install's -m MODE option works.  */
      if (chmod (dst_path, x->mode))
	{
	  error (0, errno, _("setting permissions for %s"), dst_path);
	  return 1;
	}
    }
  else if ((x->preserve_chmod_bits || new_dst)
	   && (x->copy_as_regular || S_ISREG (src_type) || S_ISDIR (src_type)))
    {
      if (chmod (dst_path, src_mode & x->umask_kill))
	{
	  error (0, errno, _("preserving permissions for %s"), dst_path);
	  if (x->require_preserve)
	    return 1;
	}
    }
  else if (fix_mode)
    {
      /* Reset the temporarily changed mode.  */
      if (chmod (dst_path, dst_sb.st_mode))
	{
	  error (0, errno, _("restoring permissions of %s"), dst_path);
	  return 1;
	}
    }

  return 0;

un_backup:
  if (dst_backup)
    {
      if (rename (dst_backup, dst_path))
	error (0, errno, _("cannot un-backup `%s'"), dst_path);
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
  return copy_internal (src_path, dst_path, nonexistent_dst, 0, NULL,
			options, move_mode, copy_into_self, rename_succeeded);
}
