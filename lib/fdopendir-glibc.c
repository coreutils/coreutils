/* fdopendir implementation derived from glibc.

   Copyright (C) 2006, 2009 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#if _LIBC
# include <dirstream.h>
# include <not-cancel.h>

#else

# if __GNUC__ < 3
#  define __builtin_expect(expr, expected_val) expr
# endif

# include "openat.h"
# define stat64 stat
# define dirent64 dirent
# define __fxstat64(V, fd, sb) fstat(fd, sb)
# define __fcntl fcntl
# define __set_errno(Val) do { errno = (Val); } while (0)
# define __libc_lock_init(NAME) ((NAME) = 0, 0)
# define close_not_cancel_no_status(fd) close (fd)
# ifdef __i386__
#  define internal_function __attribute ((regparm (3), stdcall))
# else
#  define internal_function
# endif

struct __dirstream
  {
    int fd;			/* File descriptor.  */

    char *data;			/* Directory block.  */
    size_t allocation;		/* Space allocated for the block.  */
    size_t size;		/* Total valid data in the block.  */
    size_t offset;		/* Current offset into the block.  */

    off_t filepos;		/* Position of next entry to read.  */
    int lock;
  };
#endif

#undef _STATBUF_ST_BLKSIZE

static DIR *
internal_function
__alloc_dir (int fd, bool close_fd)
{
  if (__builtin_expect (__fcntl (fd, F_SETFD, FD_CLOEXEC), 0) < 0)
    goto lose;

  size_t allocation;
#ifdef _STATBUF_ST_BLKSIZE
  if (__builtin_expect ((size_t) statp->st_blksize >= sizeof (struct dirent64),
			1))
    allocation = statp->st_blksize;
  else
#endif
    allocation = (BUFSIZ < sizeof (struct dirent64)
		  ? sizeof (struct dirent64) : BUFSIZ);

  const int pad = -sizeof (DIR) % __alignof__ (struct dirent64);

  DIR *dirp = (DIR *) malloc (sizeof (DIR) + allocation + pad);
  if (dirp == NULL)
  lose:
    {
      if (close_fd)
	{
	  int save_errno = errno;
	  close_not_cancel_no_status (fd);
	  __set_errno (save_errno);
	}
      return NULL;
    }
  memset (dirp, '\0', sizeof (DIR));
  dirp->data = (char *) (dirp + 1) + pad;
  dirp->allocation = allocation;
  dirp->fd = fd;

  __libc_lock_init (dirp->lock);

  return dirp;
}

DIR *
fdopendir (int fd)
{
#if 0
  struct stat64 statbuf;

  if (__builtin_expect (__fxstat64 (_STAT_VER, fd, &statbuf), 0) < 0)
    return NULL;
  if (__builtin_expect (! S_ISDIR (statbuf.st_mode), 0))
    {
      __set_errno (ENOTDIR);
      return NULL;
    }
  /* Make sure the descriptor allows for reading.  */
  int flags = __fcntl (fd, F_GETFL);
  if (__builtin_expect (flags == -1, 0))
    return NULL;
  if (__builtin_expect ((flags & O_ACCMODE) == O_WRONLY, 0))
    {
      __set_errno (EINVAL);
      return NULL;
    }
#endif

  return __alloc_dir (fd, false);
}
