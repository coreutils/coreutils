/* stdopen.c - ensure that the three standard file descriptors are in use

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

/* Written by Paul Eggert and Jim Meyering.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "stdopen.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/* Try to ensure that each of the standard file numbers (0, 1, 2)
   is in use.  Without this, each application would have to guard
   every call to open, dup, fopen, etc. with tests to ensure they
   don't use one of the special file numbers when opening a file.
   Return false if at least one of the file descriptors is initially
   closed and an attempt to reopen it fails.  Otherwise, return true.  */
bool
stdopen (void)
{
  int fd = dup (STDIN_FILENO);

  if (0 <= fd)
    close (fd);
  else if (errno == EBADF)
    fd = open ("/dev/null", O_WRONLY);
  else
    return false;

  if (STDIN_FILENO <= fd && fd <= STDERR_FILENO)
    {
      fd = open ("/dev/null", O_RDONLY);
      if (fd == STDOUT_FILENO)
	fd = dup (fd);
      if (fd < 0)
	return false;

      if (STDERR_FILENO < fd)
	close (fd);
    }
  return true;
}
