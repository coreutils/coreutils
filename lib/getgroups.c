/* provide consistent interface to getgroups for systems that don't allow N==0
   Copyright (C) 1996, 1999, 2003 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>

#include "xalloc.h"

/* On at least Ultrix 4.3 and NextStep 3.2, getgroups (0, 0) always fails.
   On other systems, it returns the number of supplemental groups for the
   process.  This function handles that special case and lets the system-
   provided function handle all others. */

int
getgroups (int n, GETGROUPS_T *group)
{
  int n_groups;
  GETGROUPS_T *gbuf;
  int saved_errno;

#undef getgroups

  if (n != 0)
    return getgroups (n, group);

  n = 20;
  while (1)
    {
      /* No need to worry about address arithmetic overflow here,
	 since the ancient systems that we're running on have low
	 limits on the number of secondary groups.  */
      gbuf = xmalloc (n * sizeof *gbuf);
      n_groups = getgroups (n, gbuf);
      if (n_groups < n)
	break;
      free (gbuf);
      n += 10;
    }

  saved_errno = errno;
  free (gbuf);
  errno = saved_errno;

  return n_groups;
}
