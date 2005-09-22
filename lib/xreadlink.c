/* xreadlink.c -- readlink wrapper to return the link name in malloc'd storage

   Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Jim Meyering <jim@meyering.net>  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "xreadlink.h"

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif
#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

#define MAXSIZE (SIZE_MAX < SSIZE_MAX ? SIZE_MAX : SSIZE_MAX)

#include "xalloc.h"

/* Call readlink to get the symbolic link value of FILE.
   SIZE is a hint as to how long the link is expected to be;
   typically it is taken from st_size.  It need not be correct.
   Return a pointer to that NUL-terminated string in malloc'd storage.
   If readlink fails, return NULL (caller may use errno to diagnose).
   If malloc fails, or if the link value is longer than SSIZE_MAX :-),
   give a diagnostic and exit.  */

char *
xreadlink (char const *file, size_t size)
{
  /* The initial buffer size for the link value.  A power of 2
     detects arithmetic overflow earlier, but is not required.  */
  size_t buf_size = size < MAXSIZE ? size + 1 : MAXSIZE;

  while (1)
    {
      char *buffer = xmalloc (buf_size);
      ssize_t r = readlink (file, buffer, buf_size);
      size_t link_length = r;

      /* On AIX 5L v5.3 and HP-UX 11i v2 04/09, readlink returns -1
	 with errno == ERANGE if the buffer is too small.  */
      if (r < 0 && errno != ERANGE)
	{
	  int saved_errno = errno;
	  free (buffer);
	  errno = saved_errno;
	  return NULL;
	}

      if (link_length < buf_size)
	{
	  buffer[link_length] = 0;
	  return buffer;
	}

      free (buffer);
      if (buf_size <= MAXSIZE / 2)
	buf_size *= 2;
      else if (buf_size < MAXSIZE)
	buf_size = MAXSIZE;
      else
	xalloc_die ();
    }
}
