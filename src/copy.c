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

/* On Linux (from slackware-1.2.13 to 2.0.2?) there is no lchown function.
   To change ownership of symlinks, you must run chown with an effective
   UID of 0.  */
#ifdef __linux__
# define ROOT_CHOWN_AFFECTS_SYMLINKS
#endif

#define DO_CHOWN(Chown, File, New_uid, New_gid)				\
  (Chown ((File), (x->myeuid == 0 ? (New_uid) : x->myeuid), (New_gid))	\
   /* If non-root uses -p, it's ok if we can't preserve ownership.	\
      But root probably wants to know, e.g. if NFS disallows it.  */	\
   && (errno != EPERM || x->myeuid == 0))

struct dir_list
{
  struct dir_list *parent;
  ino_t ino;
  dev_t dev;
};

int full_write ();
int euidaccess ();
int yesno ();

static int copy_internal __P ((const char *src_path, const char *dst_path,
			       int new_dst, dev_t device,
			       struct dir_list *ancestors,
			       const struct cp_options *x));

/* The invocation name of this program.  */
extern char *program_name;

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
   Return 0 if successful, -1 if an error occurs. */

static int
copy_dir (const char *src_path_in, const char *dst_path_in, int new_dst,
	  const struct stat *src_sb, struct dir_list *ancestors,
	  const struct cp_options *x)
{
  char *name_space;
  char *namep;
  char *src_path;
  char *dst_path;
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
      src_path = path_concat (src_path_in, namep, NULL);
      dst_path = path_concat (dst_path_in, namep, NULL);
      if (dst_path == NULL || src_path == NULL)
	error (1, 0, _("virtual memory exhausted"));

      ret |= copy_internal (src_path, dst_path, new_dst, src_sb->st_dev,
			    ancestors, x);

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

#ifdef HAVE_ST_BLOCKS
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
#ifdef HAVE_FTRUNCATE
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
   Return 0 if successful, 1 if an error occurs. */

static int
copy_internal (const char *src_path, const char *dst_path,
	       int new_dst, dev_t device, struct dir_list *ancestors,
	       const struct cp_options *x)
{
  struct stat src_sb;
  struct stat dst_sb;
  int src_mode;
  int src_type;
  char *earlier_file;
  char *dst_backup = NULL;
  int fix_mode = 0;

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
    return 0;

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
	    new_dst = 1;
	}
      else
	{
	  int same;

	  /* The destination file exists already.  */

	  same = (src_sb.st_ino == dst_sb.st_ino
		  && src_sb.st_dev == dst_sb.st_dev);

	  /* If we're preserving symlinks (--no-dereference) and the
	     destination file is a symlink, use stat (not xstat) to
	     see if it points back to the source.  */
	  if (!same && !x->dereference && S_ISLNK (dst_sb.st_mode))
	    {
	      struct stat dst2_sb;
	      if (stat (dst_path, &dst2_sb) == 0
		  && (src_sb.st_ino == dst2_sb.st_ino &&
		      src_sb.st_dev == dst2_sb.st_dev))
		same = 1;
	    }

	  if (same)
	    {
	      if (x->hard_link)
		return 0;

	      error (0, 0, _("`%s' and `%s' are the same file"),
		     src_path, dst_path);
	      return 1;
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

	  if (backup_type != none && !S_ISDIR (dst_sb.st_mode))
	    {
	      char *tmp_backup = find_backup_file_name (dst_path);
	      if (tmp_backup == NULL)
		error (1, 0, _("virtual memory exhausted"));

	      /* Detect (and fail) when creating the backup file would
		 destroy the source file.  Before, running the commands
		 cd /tmp; rm -f a a~; : > a; echo A > a~; cp -b -V simple a~ a
		 would leave two zero-length files: a and a~.  */
	      if (STREQ (tmp_backup, src_path))
		{
		  error (0, 0,
		   _("backing up `%s' would destroy source;  `%s' not copied"),
			 dst_path, src_path);
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
		      return 1;
		    }
		  new_dst = 1;
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

      if (copy_dir (src_path, dst_path, new_dst, &src_sb, dir, x))
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
      if (mkfifo (dst_path, src_mode & x->umask_kill))
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
      if (mknod (dst_path, src_mode & x->umask_kill, src_sb.st_rdev))
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

      if (x->preserve)
	{
	  /* Preserve the owner and group of the just-`copied'
	     symbolic link, if possible.  */
# ifdef HAVE_LCHOWN
	  if (DO_CHOWN (lchown, dst_path, src_sb.st_uid, src_sb.st_gid))
	    {
	      error (0, errno, _("preserving ownership for %s"), dst_path);
	      goto un_backup;
	    }
# else
#  ifdef ROOT_CHOWN_AFFECTS_SYMLINKS
	  if (x->myeuid == 0)
	    {
	      if (DO_CHOWN (chown, dst_path, src_sb.st_uid, src_sb.st_gid))
		{
		  error (0, errno, _("preserving ownership for %s"), dst_path);
		  goto un_backup;
		}
	    }
	  else
	    {
	      /* FIXME: maybe give a diagnostic: you must be root
		 to preserve ownership and group of symlinks.  */
	    }
#  else
	  /* Can't preserve ownership of symlinks.
	     FIXME: maybe give a warning or even error for symlinks
	     in directories with the sticky bit set -- there, not
	     preserving owner/group is a potential security problem.  */
#  endif
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

  if (x->preserve)
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

      if (DO_CHOWN (chown, dst_path, src_sb.st_uid, src_sb.st_gid))
	{
	  error (0, errno, _("preserving ownership for %s"), dst_path);
	  if (x->require_preserve)
	    return 1;
	}
    }

  if ((x->preserve || new_dst)
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

  /* FIXME: make sure xstat and dereference are consistent.  */
  assert (co->xstat);

  assert (co->sparse_mode != SPARSE_UNUSED);
  return 1;
}

/* Copy the file SRC_PATH to the file DST_PATH.  The files may be of
   any type.  NONEXISTENT_DST should be nonzero if the file DST_PATH
   is known not to exist (e.g., because its parent directory was just
   created);  NONEXISTENT_DST should be zero if DST_PATH might already
   exist.  DEVICE is the device number of the parent directory of
   DST_PATH, or 0 if the parent of this file is not known.
   OPTIONS is ... FIXME-describe
   Return 0 if successful, 1 if an error occurs. */

int
copy (const char *src_path, const char *dst_path,
      int nonexistent_dst, const struct cp_options *options)
{
  assert (valid_options (options));
  return copy_internal (src_path, dst_path, nonexistent_dst, 0, NULL, options);
}
