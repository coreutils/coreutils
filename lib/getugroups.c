/* getugroups.c -- return a list of the groups a user is in
   Copyright (C) 1990, 1991, 1998, 1999 Free Software Foundation.

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

/* Written by David MacKenzie. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h> /* grp.h on alpha OSF1 V2.0 uses "FILE *". */
#include <grp.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/* setgrent, getgrent, and endgrent are not specified by POSIX.1,
   so header files might not declare them.
   If you don't have them at all, we can't implement this function.
   You lose!  */
struct group *getgrent ();

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
#else
# include <strings.h>
#endif

/* Like `getgroups', but for user USERNAME instead of for
   the current process. */

int
getugroups (int maxcount, GETGROUPS_T *grouplist, char *username, gid_t gid)
{
  struct group *grp;
  register char **cp;
  register int count = 0;

  if (maxcount != 0)
    grouplist[count++] = gid;

  setgrent ();
  while ((grp = getgrent ()) != 0)
    for (cp = grp->gr_mem; *cp; ++cp)
      if (!strcmp (username, *cp))
	{
	  int n;

	  /* See if this group number is already on the list.  */
	  for (n = 0; n < count; ++n)
	    if (grouplist[n] == grp->gr_gid)
	      break;

	  /* If it's a new group number, then try to add it to the list.  */
	  if (n == count)
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
	}
  endgrent ();
  return count;
}
