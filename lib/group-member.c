/* group-member.c -- determine whether group id is in calling user's group list
   Copyright (C) 1994, 1997, 1998 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "group-member.h"
#include "xalloc.h"

struct group_info
  {
    int n_groups;
    GETGROUPS_T *group;
  };

#if HAVE_GETGROUPS

static void
free_group_info (struct group_info *g)
{
  free (g->group);
  free (g);
}

static struct group_info *
get_group_info (void)
{
  int n_groups;
  int n_group_slots;
  struct group_info *gi;
  GETGROUPS_T *group;

  /* getgroups () returns the number of elements that it was able to
     place into the array.  We simply continue to call getgroups ()
     until the number of elements placed into the array is smaller than
     the physical size of the array. */

  group = NULL;
  n_groups = 0;
  n_group_slots = 0;
  while (n_groups == n_group_slots)
    {
      n_group_slots += 64;
      group = (GETGROUPS_T *) xrealloc (group,
					n_group_slots * sizeof (GETGROUPS_T));
      n_groups = getgroups (n_group_slots, group);
    }

  /* In case of error, the user loses. */
  if (n_groups < 0)
    {
      free (group);
      return NULL;
    }

  gi = (struct group_info *) xmalloc (sizeof (*gi));
  gi->n_groups = n_groups;
  gi->group = group;

  return gi;
}

#endif /* not HAVE_GETGROUPS */

/* Return non-zero if GID is one that we have in our groups list.
   If there is no getgroups function, return non-zero if GID matches
   either of the current or effective group IDs.  */

int
group_member (gid_t gid)
{
#ifndef HAVE_GETGROUPS
  return ((gid == getgid ()) || (gid == getegid ()));
#else
  int i;
  int found;
  struct group_info *gi;

  gi = get_group_info ();
  if (gi == NULL)
    return 0;

  /* Search through the list looking for GID. */
  found = 0;
  for (i = 0; i < gi->n_groups; i++)
    {
      if (gid == gi->group[i])
	{
	  found = 1;
	  break;
	}
    }

  free_group_info (gi);

  return found;
#endif /* HAVE_GETGROUPS */
}

#ifdef TEST

char *program_name;

int
main (int argc, char** argv)
{
  int i;

  program_name = argv[0];

  for (i=1; i<argc; i++)
    {
      gid_t gid;

      gid = atoi (argv[i]);
      printf ("%d: %s\n", gid, group_member (gid) ? "yes" : "no");
    }
  exit (0);
}

#endif /* TEST */
