/* provide consistent interface to getgroups for systems that don't allow N==0
   Copyright (C) 1996 Free Software Foundation, Inc.

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

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

char *xmalloc ();

/* On Ultrix 4.3, getgroups (0, 0) always fails.
   This function handles that special case and lets the system-
   provided function handle all others.  */

int
getgroups (n, group)
     size_t n;
     GETGROUPS_T *group;
{
  int ng;
  GETGROUPS_T *gbuf;

#undef getgroups

  if (n != 0)
    return getgroups (n, group);

  n = 20;
  gbuf = NULL;
  while (1)
    {
      gbuf = (GETGROUPS_T *) xrealloc (gbuf, n * sizeof (GETGROUPS_T));
      ng = getgroups (n, gbuf);
      if (ng < n)
	break;
      n += 10;
    }

  free (gbuf);

  return ng;
}
