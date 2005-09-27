/* Copyright (C) 1991,92,93,94,95,96,97,98,99,2004,2005 Free Software
   Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if !_LIBC
# include "getcwd.h"
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stddef.h>

#include <fcntl.h> /* For AT_FDCWD on Solaris 9.  */

#ifndef __set_errno
# define __set_errno(val) (errno = (val))
#endif

#if HAVE_DIRENT_H || _LIBC
# include <dirent.h>
# ifndef _D_EXACT_NAMLEN
#  define _D_EXACT_NAMLEN(d) strlen ((d)->d_name)
# endif
#else
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#ifndef _D_EXACT_NAMLEN
# define _D_EXACT_NAMLEN(d) ((d)->d_namlen)
#endif
#ifndef _D_ALLOC_NAMLEN
# define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN (d) + 1)
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#if _LIBC
# ifndef mempcpy
#  define mempcpy __mempcpy
# endif
#else
# include "mempcpy.h"
#endif

#include <limits.h>

#ifdef ENAMETOOLONG
# define is_ENAMETOOLONG(x) ((x) == ENAMETOOLONG)
#else
# define is_ENAMETOOLONG(x) 0
#endif

#ifndef MAX
# define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif
#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef PATH_MAX
# ifdef	MAXPATHLEN
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 1024
# endif
#endif

#if D_INO_IN_DIRENT
# define MATCHING_INO(dp, ino) ((dp)->d_ino == (ino))
#else
# define MATCHING_INO(dp, ino) true
#endif

#if !_LIBC
# define __getcwd getcwd
# define __lstat lstat
# define __closedir closedir
# define __opendir opendir
# define __readdir readdir
#endif

/* Get the name of the current working directory, and put it in SIZE
   bytes of BUF.  Returns NULL if the directory couldn't be determined or
   SIZE was too small.  If successful, returns BUF.  In GNU, if BUF is
   NULL, an array is allocated with `malloc'; the array is SIZE bytes long,
   unless SIZE == 0, in which case it is as big as necessary.  */

char *
__getcwd (char *buf, size_t size)
{
  /* Lengths of big file name components and entire file names, and a
     deep level of file name nesting.  These numbers are not upper
     bounds; they are merely large values suitable for initial
     allocations, designed to be large enough for most real-world
     uses.  */
  enum
    {
      BIG_FILE_NAME_COMPONENT_LENGTH = 255,
      BIG_FILE_NAME_LENGTH = MIN (4095, PATH_MAX - 1),
      DEEP_NESTING = 100
    };

#ifdef AT_FDCWD
  int fd = AT_FDCWD;
  bool fd_needs_closing = false;
#else
  char dots[DEEP_NESTING * sizeof ".." + BIG_FILE_NAME_COMPONENT_LENGTH + 1];
  char *dotlist = dots;
  size_t dotsize = sizeof dots;
  size_t dotlen = 0;
#endif
  DIR *dirstream = NULL;
  dev_t rootdev, thisdev;
  ino_t rootino, thisino;
  char *dir;
  register char *dirp;
  struct stat st;
  size_t allocated = size;
  size_t used;

#if HAVE_PARTLY_WORKING_GETCWD && !defined AT_FDCWD
  /* The system getcwd works, except it sometimes fails when it
     shouldn't, setting errno to ERANGE, ENAMETOOLONG, or ENOENT.  If
     AT_FDCWD is not defined, the algorithm below is O(N**2) and this
     is much slower than the system getcwd (at least on GNU/Linux).
     So trust the system getcwd's results unless they look
     suspicious.  */
# undef getcwd
  dir = getcwd (buf, size);
  if (dir || (errno != ERANGE && !is_ENAMETOOLONG (errno) && errno != ENOENT))
    return dir;
#endif

  if (size == 0)
    {
      if (buf != NULL)
	{
	  __set_errno (EINVAL);
	  return NULL;
	}

      allocated = BIG_FILE_NAME_LENGTH + 1;
    }

  if (buf == NULL)
    {
      dir = malloc (allocated);
      if (dir == NULL)
	return NULL;
    }
  else
    dir = buf;

  dirp = dir + allocated;
  *--dirp = '\0';

  if (__lstat (".", &st) < 0)
    goto lose;
  thisdev = st.st_dev;
  thisino = st.st_ino;

  if (__lstat ("/", &st) < 0)
    goto lose;
  rootdev = st.st_dev;
  rootino = st.st_ino;

  while (!(thisdev == rootdev && thisino == rootino))
    {
      struct dirent *d;
      dev_t dotdev;
      ino_t dotino;
      bool mount_point;
      int parent_status;

      /* Look at the parent directory.  */
#ifdef AT_FDCWD
      fd = openat (fd, "..", O_RDONLY);
      if (fd < 0)
	goto lose;
      fd_needs_closing = true;
      parent_status = fstat (fd, &st);
#else
      dotlist[dotlen++] = '.';
      dotlist[dotlen++] = '.';
      dotlist[dotlen] = '\0';
      parent_status = __lstat (dotlist, &st);
#endif
      if (parent_status != 0)
	goto lose;

      if (dirstream && __closedir (dirstream) != 0)
	{
	  dirstream = NULL;
	  goto lose;
	}

      /* Figure out if this directory is a mount point.  */
      dotdev = st.st_dev;
      dotino = st.st_ino;
      mount_point = dotdev != thisdev;

      /* Search for the last directory.  */
#ifdef AT_FDCWD
      dirstream = fdopendir (fd);
      if (dirstream == NULL)
	goto lose;
      fd_needs_closing = false;
#else
      dirstream = __opendir (dotlist);
      if (dirstream == NULL)
	goto lose;
      dotlist[dotlen++] = '/';
#endif
      /* Clear errno to distinguish EOF from error if readdir returns
	 NULL.  */
      __set_errno (0);
      while ((d = __readdir (dirstream)) != NULL)
	{
	  if (d->d_name[0] == '.' &&
	      (d->d_name[1] == '\0' ||
	       (d->d_name[1] == '.' && d->d_name[2] == '\0')))
	    continue;
	  if (MATCHING_INO (d, thisino) || mount_point)
	    {
	      int entry_status;
#ifdef AT_FDCWD
	      entry_status = fstatat (fd, d->d_name, &st, AT_SYMLINK_NOFOLLOW);
#else
	      /* Compute size needed for this file name, or for the file
		 name ".." in the same directory, whichever is larger.
	         Room for ".." might be needed the next time through
		 the outer loop.  */
	      size_t name_alloc = _D_ALLOC_NAMLEN (d);
	      size_t filesize = dotlen + MAX (sizeof "..", name_alloc);

	      if (filesize < dotlen)
		goto memory_exhausted;

	      if (dotsize < filesize)
		{
		  /* My, what a deep directory tree you have, Grandma.  */
		  size_t newsize = MAX (filesize, dotsize * 2);
		  size_t i;
		  if (newsize < dotsize)
		    goto memory_exhausted;
		  if (dotlist != dots)
		    free (dotlist);
		  dotlist = malloc (newsize);
		  if (dotlist == NULL)
		    goto lose;
		  dotsize = newsize;

		  i = 0;
		  do
		    {
		      dotlist[i++] = '.';
		      dotlist[i++] = '.';
		      dotlist[i++] = '/';
		    }
		  while (i < dotlen);
		}

	      strcpy (dotlist + dotlen, d->d_name);
	      entry_status = __lstat (dotlist, &st);
#endif
	      /* We don't fail here if we cannot stat() a directory entry.
		 This can happen when (network) file systems fail.  If this
		 entry is in fact the one we are looking for we will find
		 out soon as we reach the end of the directory without
		 having found anything.  */
	      if (entry_status == 0 && S_ISDIR (st.st_mode)
		  && st.st_dev == thisdev && st.st_ino == thisino)
		break;
	    }
	}
      if (d == NULL)
	{
	  if (errno == 0)
	    /* EOF on dirstream, which means that the current directory
	       has been removed.  */
	    __set_errno (ENOENT);
	  goto lose;
	}
      else
	{
	  size_t dirroom = dirp - dir;
	  size_t namlen = _D_EXACT_NAMLEN (d);

	  if (dirroom <= namlen)
	    {
	      if (size != 0)
		{
		  __set_errno (ERANGE);
		  goto lose;
		}
	      else
		{
		  char *tmp;
		  size_t oldsize = allocated;

		  allocated += MAX (allocated, namlen);
		  if (allocated < oldsize
		      || ! (tmp = realloc (dir, allocated)))
		    goto memory_exhausted;

		  /* Move current contents up to the end of the buffer.
		     This is guaranteed to be non-overlapping.  */
		  dirp = memcpy (tmp + allocated - (oldsize - dirroom),
				 tmp + dirroom,
				 oldsize - dirroom);
		  dir = tmp;
		}
	    }
	  dirp -= namlen;
	  memcpy (dirp, d->d_name, namlen);
	  *--dirp = '/';
	}

      thisdev = dotdev;
      thisino = dotino;
    }

  if (dirstream && __closedir (dirstream) != 0)
    {
      dirstream = NULL;
      goto lose;
    }

  if (dirp == &dir[allocated - 1])
    *--dirp = '/';

#ifndef AT_FDCWD
  if (dotlist != dots)
    free (dotlist);
#endif

  used = dir + allocated - dirp;
  memmove (dir, dirp, used);

  if (buf == NULL && size == 0)
    /* Ensure that the buffer is only as large as necessary.  */
    buf = realloc (dir, used);

  if (buf == NULL)
    /* Either buf was NULL all along, or `realloc' failed but
       we still have the original string.  */
    buf = dir;

  return buf;

 memory_exhausted:
  __set_errno (ENOMEM);
 lose:
  {
    int save = errno;
    if (dirstream)
      __closedir (dirstream);
#ifdef AT_FDCWD
    if (fd_needs_closing)
      close (fd);
#else
    if (dotlist != dots)
      free (dotlist);
#endif
    if (buf == NULL)
      free (dir);
    __set_errno (save);
  }
  return NULL;
}

#ifdef weak_alias
weak_alias (__getcwd, getcwd)
#endif
