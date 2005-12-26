/* much like chdir(2), but safer

   Copyright (C) 2005 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "chdir-safer.h"

#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef O_DIRECTORY
# define O_DIRECTORY 0
#endif

#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

#define SAME_INODE(Stat_buf_1, Stat_buf_2) \
  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

/* Like chdir, but fail if DIR is a symbolic link to a directory (or
   similar funny business), or if DIR is neither readable nor
   writable.  This avoids a minor race condition between when a
   directory is created or statted and when the process chdirs into it.

   On some systems, a writable yet unreadable directory cannot be
   opened via open with O_WRONLY.  For example, on Linux 2.6, the
   open syscall fails with EISDIR.  */
int
chdir_no_follow (char const *dir)
{
  int result = 0;
  int saved_errno;
  int open_flags = O_DIRECTORY | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK;
  int fd = open (dir, O_RDONLY | open_flags);
  if (fd < 0)
    {
      if (errno == EACCES)
	fd = open (dir, O_WRONLY | open_flags);
      if (fd < 0)
	return fd;
    }

  /* If open follows symlinks, lstat DIR and fstat FD to ensure that
     they are the same file; if they are different files, set errno to
     ELOOP (the same value that open uses for symlinks with
     O_NOFOLLOW) so the caller can report a failure.  */
  if (! O_NOFOLLOW)
    {
      struct stat sb1;
      result = lstat (dir, &sb1);
      if (result == 0)
	{
	  struct stat sb2;
	  result = fstat (fd, &sb2);
	  if (result == 0 && ! SAME_INODE (sb1, sb2))
	    {
	      errno = ELOOP;
	      result = -1;
	    }
	}
    }

  if (result == 0)
    result = fchdir (fd);

  saved_errno = errno;
  close (fd);
  errno = saved_errno;
  return result;
}
