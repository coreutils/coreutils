/* xgetcwd.c -- return current directory with unlimited length
   Copyright (C) 1992, 1996, 2000, 2001 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <sys/types.h>

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_GETCWD
char *getcwd ();
#else
char *getwd ();
# define getcwd(Buf, Max) getwd (Buf)
#endif

#include "xalloc.h"

/* Return the current directory, newly allocated, arbitrarily long.
   Return NULL and set errno on error. */

char *
xgetcwd ()
{
#if defined __GLIBC__ && __GLIBC__ >= 2
  return getcwd (NULL, 0);
#else
  size_t buf_size = 128;  /* must be a power of 2 */
  char *buf = NULL;

  while (1)
    {
      char *ret;
      buf = (char *) xrealloc (buf, buf_size);

      cwd = getcwd (buf, buf_size);
      if (cwd != NULL)
	return cwd;
      if (errno != ERANGE)
	{
	  free (buf);
	  return NULL;
	}

      buf_size *= 2;
      if (buf_size == 0)
	xalloc_die ();
    }
#endif
}
