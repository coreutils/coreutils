/* chown-core.c -- core functions for changing ownership.
   Copyright (C) 2000 Free Software Foundation.

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

/* Extracted from chown.c/chgrp.c and librarified by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "system.h"
#include "error.h"
#include "lchown.h"
#include "quote.h"
#include "savedir.h"
#include "chown-core.h"
#include "xalloc.h"

#ifndef _POSIX_VERSION
struct group *getgrnam ();
struct group *getgrgid ();
#endif

void
chopt_init (struct Chown_option *chopt)
{
  chopt->verbosity = V_off;
  chopt->change_symlinks = 1;
  chopt->recurse = 0;
  chopt->force_silent = 0;
  chopt->user_name = 0;
  chopt->group_name = 0;
}

void
chopt_free (struct Chown_option *chopt)
{
  XFREE (chopt->user_name);
  XFREE (chopt->group_name);
}

/* FIXME: describe */

char *
gid_to_name (gid_t gid)
{
  struct group *grp = getgrgid (gid);
  return xstrdup (grp == NULL ? _("<unknown>") : grp->gr_name);
}

char *
uid_to_name (uid_t uid)
{
  struct passwd *pwd = getpwuid (uid);
  return xstrdup (pwd == NULL ? _("<unknown>") : pwd->pw_name);
}

/* Tell the user how/if the user and group of FILE have been changed.
   If USER is NULL, give the group-oriented messages.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, enum Change_status changed,
		 char const *user, char const *group)
{
  const char *fmt;

  if (changed == CH_NOT_APPLIED)
    {
      printf (_("neither symbolic link %s nor referent has been changed\n"),
	      quote (file));
      return;
    }

  if (user == NULL)
    {
      switch (changed)
	{
	case CH_SUCCEEDED:
	  fmt = _("group of %s changed to %s\n");
	  break;
	case CH_FAILED:
	  fmt = _("failed to change group of %s to %s\n");
	  break;
	case CH_NO_CHANGE_REQUESTED:
	  fmt = _("group of %s retained as %s\n");
	  break;
	default:
	  abort ();
	}
      printf (fmt, quote (file), group);
    }
  else
    {
      switch (changed)
	{
	case CH_SUCCEEDED:
	  fmt = _("ownership of %s changed to ");
	  break;
	case CH_FAILED:
	  fmt = _("failed to change ownership of %s to ");
	  break;
	case CH_NO_CHANGE_REQUESTED:
	  fmt = _("ownership of %s retained as ");
	  break;
	default:
	  abort ();
	}
      printf (fmt, file);
      if (group)
	printf ("%s.%s\n", user, group);
      else
	printf ("%s\n", user);
    }
}

/* Recursively change the ownership of the files in directory DIR
   to UID USER and GID GROUP, according to the options specified by CHOPT.
   STATP points to the results of lstat on DIR.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_owner (const char *dir, uid_t user, gid_t group,
		  uid_t old_user, gid_t old_group,
		  const struct stat *statp,
		  struct Chown_option const *chopt)
{
  char *name_space, *namep;
  char *path;			/* Full path of each entry to process. */
  unsigned dirlength;		/* Length of `dir' and '\0'. */
  unsigned filelength;		/* Length of each pathname to process. */
  unsigned pathlength;		/* Bytes allocated for `path'. */
  int errors = 0;

  name_space = savedir (dir, statp->st_size);
  if (name_space == NULL)
    {
      if (chopt->force_silent == 0)
	error (0, errno, "%s", quote (dir));
      return 1;
    }

  dirlength = strlen (dir) + 1;	/* + 1 is for the trailing '/'. */
  pathlength = dirlength + 1;
  /* Give `path' a dummy value; it will be reallocated before first use. */
  path = xmalloc (pathlength);
  strcpy (path, dir);
  path[dirlength - 1] = '/';

  for (namep = name_space; *namep; namep += filelength - dirlength)
    {
      filelength = dirlength + strlen (namep) + 1;
      if (filelength > pathlength)
	{
	  pathlength = filelength * 2;
	  path = xrealloc (path, pathlength);
	}
      strcpy (path + dirlength, namep);
      errors |= change_file_owner (0, path, user, group, old_user, old_group,
				   chopt);
    }
  free (path);
  free (name_space);
  return errors;
}

/* Change the ownership of FILE to UID USER and GID GROUP
   provided it presently has UID OLDUSER and GID OLDGROUP.
   Honor the options specified by CHOPT.
   If FILE is a directory and -R is given, recurse.
   Return 0 if successful, 1 if errors occurred. */

int
change_file_owner (int cmdline_arg, const char *file, uid_t user, gid_t group,
		   uid_t old_user, gid_t old_group,
		   struct Chown_option const *chopt)
{
  struct stat file_stats;
  uid_t newuser;
  gid_t newgroup;
  int errors = 0;

  if (lstat (file, &file_stats))
    {
      if (chopt->force_silent == 0)
	error (0, errno, _("getting attributes of %s"), quote (file));
      return 1;
    }

  if ((old_user == (uid_t) -1  || file_stats.st_uid == old_user) &&
      (old_group == (gid_t) -1 || file_stats.st_gid == old_group))
    {
      newuser = user == (uid_t) -1 ? file_stats.st_uid : user;
      newgroup = group == (gid_t) -1 ? file_stats.st_gid : group;
      if (newuser != file_stats.st_uid || newgroup != file_stats.st_gid)
	{
	  int fail;
	  int symlink_changed = 1;
	  int saved_errno;

	  if (S_ISLNK (file_stats.st_mode) && chopt->change_symlinks)
	    {
	      fail = lchown (file, newuser, newgroup);

	      /* Ignore the failure if it's due to lack of support (ENOSYS)
		 and this is not a command line argument.  */
	      if (!cmdline_arg && fail && errno == ENOSYS)
		{
		  fail = 0;
		  symlink_changed = 0;
		}
	    }
	  else
	    {
	      fail = chown (file, newuser, newgroup);
	    }
	  saved_errno = errno;

	  if (chopt->verbosity == V_high
	      || (chopt->verbosity == V_changes_only && !fail))
	    {
	      enum Change_status ch_status = (! symlink_changed
					      ? CH_NOT_APPLIED
					      : (fail
						 ? CH_FAILED : CH_SUCCEEDED));
	      describe_change (file, ch_status,
			       chopt->user_name, chopt->group_name);
	    }

	  if (fail)
	    {
	      if (chopt->force_silent == 0)
		error (0, saved_errno, _("changing ownership of %s"),
		       quote (file));
	      errors = 1;
	    }
	  else
	    {
	      /* The change succeeded.  On some systems, the chown function
		 resets the `special' permission bits.  When run by a
		 `privileged' user, this program must ensure that at least
		 the set-uid and set-group ones are still set.  */
	      if (file_stats.st_mode & ~S_IRWXUGO
		  /* If this is a symlink and we changed *it*, then skip it.  */
		  && ! (S_ISLNK (file_stats.st_mode) && chopt->change_symlinks))
		{
		  if (chmod (file, file_stats.st_mode))
		    {
		      error (0, saved_errno,
			     _("unable to restore permissions of %s"),
			     quote (file));
		      fail = 1;
		    }
		}
	    }
	}
      else if (chopt->verbosity == V_high)
	{
	  describe_change (file, CH_NO_CHANGE_REQUESTED,
			   chopt->user_name, chopt->group_name);
	}
    }

  if (chopt->recurse && S_ISDIR (file_stats.st_mode))
    errors |= change_dir_owner (file, user, group,
				old_user, old_group, &file_stats, chopt);
  return errors;
}
