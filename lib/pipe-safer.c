/* Invoke pipe, but avoid some glitches.
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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "unistd-safer.h"

#include <unistd.h>

/* Like pipe, but ensure that neither of the file descriptors is
   STDIN_FILENO, STDOUT_FILENO, or STDERR_FILENO.  */

int
pipe_safer (int fd[2])
{
  int fail = pipe (fd);
  if (fail)
    return fail;

  {
    int i;
    for (i = 0; i < 2; i++)
      {
	int f = fd_safer (fd[i]);
	if (f < 0)
	  return -1;
	fd[i] = f;
      }
  }

  return 0;
}
