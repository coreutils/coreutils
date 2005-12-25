/* like chdir(2), but safer, if possible
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
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fcntl--.h" /* for the open->open_safer mapping */

#if !defined O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

#define SAME_INODE(Stat_buf_1, Stat_buf_2) \
  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

/* Just like chmod, but fail if DIR is a symbolic link.
   This can avoid a minor race condition between when a
   directory is created or stat'd and when we chdir into it.

   Note that this function fails (while chdir would succeed)
   if DIR cannot be opened with O_RDONLY.  */
int
chdir_no_follow (char const *dir)
{
  int fail = -1;
  struct stat sb;
  struct stat sb_init;
  int saved_errno = 0;
  int fd;

  bool open_dereferences_symlink = ! O_NOFOLLOW;

  /* If open follows symlinks, lstat DIR, to get its device and
     inode numbers.  */
  if (open_dereferences_symlink && lstat (dir, &sb_init) != 0)
    return fail;

  fd = open (dir, O_NOFOLLOW | O_RDONLY | O_NDELAY);

  if (0 <= fd
      && fstat (fd, &sb) == 0
      /* If DIR is a different directory, then someone is trying to do
	 something nasty.  However, the risk of such an attack is so low
	 that it isn't worth a special diagnostic.  Simply skip the fchdir
	 and set errno (to the same value that open uses for symlinks with
	 O_NOFOLLOW), so that the caller can report the failure.  */
      && ( ! open_dereferences_symlink || SAME_INODE (sb_init, sb)
	  || ((errno = ELOOP), 0))
      && fchdir (fd) == 0)
    {
      fail = 0;
    }
  else
    {
      saved_errno = errno;
    }

  if (0 < fd)
    close (fd); /* Ignore any failure.  */

  errno = saved_errno;
  return fail;
}
