/* mgetgroups.c -- return a list of the groups a user is in

   Copyright (C) 2007-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Extracted from coreutils' src/id.c. */

#include <config.h>

#include "mgetgroups.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#if HAVE_GETGROUPLIST
# include <grp.h>
#endif
#include "getugroups.h"
#include "xalloc.h"


static GETGROUPS_T *
realloc_groupbuf (GETGROUPS_T *g, size_t num)
{
  if (xalloc_oversized (num, sizeof (*g)))
    {
      errno = ENOMEM;
      return NULL;
    }

  return realloc (g, num * sizeof (*g));
}

/* Like getugroups, but store the result in malloc'd storage.
   Set *GROUPS to the malloc'd list of all group IDs of which USERNAME
   is a member.  If GID is not -1, store it first.  GID should be the
   group ID (pw_gid) obtained from getpwuid, in case USERNAME is not
   listed in the groups database (e.g., /etc/groups).  Upon failure,
   don't modify *GROUPS, set errno, and return -1.  Otherwise, return
   the number of groups.  */

int
mgetgroups (char const *username, gid_t gid, GETGROUPS_T **groups)
{
  int max_n_groups;
  int ng;
  GETGROUPS_T *g;

#if HAVE_GETGROUPLIST
  /* We prefer to use getgrouplist if available, because it has better
     performance characteristics.

     In glibc 2.3.2, getgrouplist is buggy.  If you pass a zero as the
     length of the output buffer, getgrouplist will still write to the
     buffer.  Contrary to what some versions of the getgrouplist
     manpage say, this doesn't happen with nonzero buffer sizes.
     Therefore our usage here just avoids a zero sized buffer.  */
  if (username)
    {
      enum { N_GROUPS_INIT = 10 };
      max_n_groups = N_GROUPS_INIT;

      g = realloc_groupbuf (NULL, max_n_groups);
      if (g == NULL)
        return -1;

      while (1)
        {
          GETGROUPS_T *h;
          int last_n_groups = max_n_groups;

          /* getgrouplist updates max_n_groups to num required.  */
          ng = getgrouplist (username, gid, g, &max_n_groups);

          /* Some systems (like Darwin) have a bug where they
             never increase max_n_groups.  */
          if (ng < 0 && last_n_groups == max_n_groups)
            max_n_groups *= 2;

          if ((h = realloc_groupbuf (g, max_n_groups)) == NULL)
            {
              int saved_errno = errno;
              free (g);
              errno = saved_errno;
              return -1;
            }
          g = h;

          if (0 <= ng)
            {
              *groups = g;
              /* On success some systems just return 0 from getgrouplist,
                 so return max_n_groups rather than ng.  */
              return max_n_groups;
            }
        }
    }
  /* else no username, so fall through and use getgroups. */
#endif

  max_n_groups = (username
                  ? getugroups (0, NULL, username, gid)
                  : getgroups (0, NULL));

  /* If we failed to count groups with NULL for a buffer,
     try again with a non-NULL one, just in case.  */
  if (max_n_groups < 0)
      max_n_groups = 5;

  g = realloc_groupbuf (NULL, max_n_groups);
  if (g == NULL)
    return -1;

  ng = (username
        ? getugroups (max_n_groups, g, username, gid)
        : getgroups (max_n_groups, g));

  if (ng < 0)
    {
      int saved_errno = errno;
      free (g);
      errno = saved_errno;
      return -1;
    }

  *groups = g;
  return ng;
}
