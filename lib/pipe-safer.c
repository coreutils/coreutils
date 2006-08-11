/* Invoke pipe, but avoid some glitches.
   Copyright (C) 2005, 2006 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "unistd-safer.h"

#include <unistd.h>
#include <errno.h>

/* Like pipe, but ensure that neither of the file descriptors is
   STDIN_FILENO, STDOUT_FILENO, or STDERR_FILENO.  Fail with ENOSYS on
   platforms that lack pipe.  */

int
pipe_safer (int fd[2])
{
#if HAVE_PIPE
  if (pipe (fd) == 0)
    {
      int i;
      for (i = 0; i < 2; i++)
	{
	  fd[i] = fd_safer (fd[i]);
	  if (fd[i] < 0)
	    {
	      int e = errno;
	      close (fd[1 - i]);
	      errno = e;
	      return -1;
	    }
	}

      return 0;
    }
#else
  errno = ENOSYS;
#endif

  return -1;
}
