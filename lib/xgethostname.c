/* xgethostname.c -- return current hostname with unlimited length
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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "error.h"
#include "xalloc.h"

#ifndef ENAMETOOLONG
# define ENAMETOOLONG 9999
#endif

#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

int gethostname ();

#ifndef INITIAL_HOSTNAME_LENGTH
# define INITIAL_HOSTNAME_LENGTH 34
#endif

char *
xgethostname ()
{
  char *hostname;
  size_t size;

  size = INITIAL_HOSTNAME_LENGTH;
  /* Use size + 1 here rather than size to work around the bug
     in SunOS5.5's gethostname whereby it NUL-terminates HOSTNAME
     even when the name is longer than the supplied buffer.  */
  hostname = xmalloc (size + 1);
  while (1)
    {
      int k = size - 1;
      int err;

      errno = 0;
      hostname[k] = '\0';
      err = gethostname (hostname, size);
      if (err >= 0 && hostname[k] == '\0')
	break;
      else if (err < 0 && errno != ENAMETOOLONG && errno != 0)
	error (EXIT_FAILURE, errno, "gethostname");
      size *= 2;
      hostname = xrealloc (hostname, size + 1);
    }

  return hostname;
}
