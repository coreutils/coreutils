/* xgethostname.c -- return current hostname with unlimited length

   Copyright (C) 1992, 1996, 2000, 2001, 2003, 2004 Free Software
   Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "xgethostname.h"

#include <stdlib.h>
#include <errno.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "xalloc.h"

#ifndef ENAMETOOLONG
# define ENAMETOOLONG 0
#endif

#ifndef INITIAL_HOSTNAME_LENGTH
# define INITIAL_HOSTNAME_LENGTH 34
#endif

/* Return the current hostname in malloc'd storage.
   If malloc fails, exit.
   Upon any other failure, return NULL and set errno.  */
char *
xgethostname (void)
{
  char *hostname = NULL;
  size_t size = INITIAL_HOSTNAME_LENGTH;

  while (1)
    {
      /* Use SIZE_1 here rather than SIZE to work around the bug in
	 SunOS 5.5's gethostname whereby it NUL-terminates HOSTNAME
	 even when the name is as long as the supplied buffer.  */
      size_t size_1;

      hostname = x2realloc (hostname, &size);
      size_1 = size - 1;
      hostname[size_1 - 1] = '\0';
      errno = 0;

      if (gethostname (hostname, size_1) == 0)
	{
	  if (! hostname[size_1 - 1])
	    break;
	}
      else if (errno != 0 && errno != ENAMETOOLONG && errno != EINVAL)
	{
	  int saved_errno = errno;
	  free (hostname);
	  errno = saved_errno;
	  return NULL;
	}
    }

  return hostname;
}
