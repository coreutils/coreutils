/* Duplicate an open file descriptor to a specified file descriptor.
   Copyright (C) 1999, 2004, 2005 Free Software Foundation, Inc.

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

/* written by Paul Eggert */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef F_DUPFD
static int
dupfd (int fd, int desired_fd)
{
  int duplicated_fd = dup (fd);
  if (duplicated_fd < 0 || duplicated_fd == desired_fd)
    return duplicated_fd;
  else
    {
      int r = dupfd (fd, desired_fd);
      int e = errno;
      close (duplicated_fd);
      errno = e;
      return r;
    }
}
#endif

int
dup2 (int fd, int desired_fd)
{
  if (fd == desired_fd)
    return fd;
  close (desired_fd);
#ifdef F_DUPFD
  return fcntl (fd, F_DUPFD, desired_fd);
#else
  return dupfd (fd, desired_fd);
#endif
}
