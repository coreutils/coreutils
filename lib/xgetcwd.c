/* xgetcwd.c -- return current directory with unlimited length
   Copyright (C) 1992, 1996, 2000 Free Software Foundation, Inc.

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
#include "pathmax.h"

#if HAVE_GETCWD
char *getcwd ();
#else
char *getwd ();
# define getcwd(Buf, Max) getwd (Buf)
#endif

extern void *xmalloc ();
extern char *xstrdup ();
extern void free ();

/* Return the current directory, newly allocated, arbitrarily long.
   Return NULL and set errno on error. */

char *
xgetcwd ()
{
  char *ret;
  unsigned path_max;
  char buf[1024];

  errno = 0;
  ret = getcwd (buf, sizeof (buf));
  if (ret != NULL)
    return xstrdup (buf);
  if (errno != ERANGE)
    return NULL;

  path_max = (unsigned) PATH_MAX;
  path_max += 2;		/* The getcwd docs say to do this. */

  for (;;)
    {
      char *cwd = (char *) xmalloc (path_max);

      errno = 0;
      ret = getcwd (cwd, path_max);
      if (ret != NULL)
	return ret;
      if (errno != ERANGE)
	{
	  int save_errno = errno;
	  free (cwd);
	  errno = save_errno;
	  return NULL;
	}

      free (cwd);

      path_max += path_max / 16;
      path_max += 32;
    }
}
