/* getugroups.c -- return a list of the groups a user is in
   Copyright (C) 1990, 1991 Free Software Foundation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by David MacKenzie. */

#include <sys/types.h>
#include <grp.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Even though SunOS 4, Ultrix 4, and 386BSD are mostly POSIX.1 compliant,
   their getgroups system call (except in the `System V' environment, which
   is troublesome in other ways) fills in an array of int, not gid_t
   (which is `short' on those systems).  We do the same, for consistency.
   Kludge, kludge.  */

#ifdef _POSIX_VERSION
#if !defined(sun) && !defined(ultrix) && !defined(__386BSD__)
#define GETGROUPS_T gid_t
#else /* sun or ultrix or 386BSD */
#define GETGROUPS_T int
#endif /* sun or ultrix or 386BSD */
#else /* not _POSIX_VERSION */
#define GETGROUPS_T int
#endif /* not _POSIX_VERSION */

/* setgrent, getgrent, and endgrent are not specified by POSIX.1,
   so header files might not declare them.
   If you don't have them at all, we can't implement this function.
   You lose!  */
struct group *getgrent ();

#if defined(USG) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#endif

/* Like `getgroups', but for user USERNAME instead of for
   the current process. */

int
getugroups (maxcount, grouplist, username)
     int maxcount;
     GETGROUPS_T *grouplist;
     char *username;
{
  struct group *grp;
  register char **cp;
  register int count = 0;

  setgrent ();
  while ((grp = getgrent ()) != 0)
    for (cp = grp->gr_mem; *cp; ++cp)
      if (!strcmp (username, *cp))
	{
	  if (maxcount != 0)
	    {
	      if (count >= maxcount)
		{
		  endgrent ();
		  return count;
		}
	      grouplist[count] = grp->gr_gid;
	    }
	  count++;
	}
  endgrent ();
  return count;
}
