/* Invoke open, but avoid some glitches.

   Copyright (C) 2004 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl-safer.h>

#include <unistd-safer.h>

#include <errno.h>
#include <stdarg.h>

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/* Like open, but do not return STDIN_FILENO, STDOUT_FILENO, or
   STDERR_FILENO.  */

int
open_safer (char const *file, int oflag, ...)
{
  int fd;
  mode_t mode = 0;

  if (oflag & O_CREAT)
    {
      va_list args;
      va_start (args, oflag);
      if (sizeof (int) <= sizeof (mode_t))
	mode = va_arg (args, mode_t);
      else
	mode = va_arg (args, int);
      va_end (args);
    }

  fd = open (file, oflag, mode);

  if (0 <= fd && fd <= STDERR_FILENO)
    {
      int f = dup_safer (fd);
      int e = errno;
      close (fd);
      errno = e;
      fd = f;
    }

  return fd;
}
