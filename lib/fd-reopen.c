/* Invoke open, but return either a desired file descriptor or -1.

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

/* Written by Paul Eggert.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fd-reopen.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* Open a file to a particular file descriptor.  This is like standard
   `open', except it always returns DESIRED_FD if successful.  */

int
fd_reopen (int desired_fd, char const *file, int flags, mode_t mode)
{
  int fd;

  close (desired_fd);
  fd = open (file, flags, mode);
  if (fd == desired_fd || fd < 0)
    return fd;
  else
    {
      int fd2 = fcntl (fd, F_DUPFD, desired_fd);
      int saved_errno = errno;
      close (fd);
      errno = saved_errno;
      return fd2;
    }
}
