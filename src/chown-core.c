/* chown-core.c -- core functions for changing ownership.
   Copyright (C) 2000, 2002 Free Software Foundation.

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

/* The number of decimal digits required to represent the largest value of
   type `unsigned int'.  This is enough for an 8-byte unsigned int type.  */
#define UINT_MAX_DECIMAL_DIGITS 20

#ifndef _POSIX_VERSION
struct group *getgrnam ();
struct group *getgrgid ();
#endif

int lstat ();

void
chopt_init (struct Chown_option *chopt)
{
  chopt->verbosity = V_off;
  chopt->dereference = DEREF_NEVER;
  chopt->recurse = 0;
  chopt->force_silent = 0;
  chopt->user_name = 0;
  chopt->group_name = 0;
}

void
chopt_free (struct Chown_option *chopt)
{
  /* Deliberately do not free chopt->user_name or ->group_name.
     They're not always allocated.  */
}

/* Convert N to a string, and return a pointer to that string in memory
   allocated from the heap.  */

static char *
uint_to_string (unsigned int n)
{
  char buf[UINT_MAX_DECIMAL_DIGITS + 1];
  char *p = buf + sizeof buf;

  *--p = '\0';

  do
    *--p = '0' + (n % 10);
  while ((n /= 10) != 0);

  return xstrdup (p);
}

/* Convert the numeric group-id, GID, to a string stored in xmalloc'd memory,
   and return it.  If there's no corresponding group name, use the decimal
   representation of the ID.  */

char *
gid_to_name (gid_t gid)
{
  struct group *grp = getgrgid (gid);
  return grp ? xstrdup (grp->gr_name) : uint_to_string (gid);
}

/* Convert the numeric user-id, UID, to a string stored in xmalloc'd memory,
   and return it.  If there's no corresponding user name, use the decimal
   representation of the ID.  */

char *
uid_to_name (uid_t uid)
{
  struct passwd *pwd = getpwuid (uid);
  return pwd ? xstrdup (pwd->pw_name) : uint_to_string (uid);
}

/* Tell the user how/if the user and group of FILE have been changed.
   If USER is NULL, give the group-oriented messages.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, enum Change_status changed,
		 char const *user, char const *group)
{
  const char *fmt;
  char *spec;
  int spec_allocated = 0;

  if (changed == CH_NOT_APPLIED)
    {
      printf (_("neither symbolic link %s nor referent has been changed\n"),
	      quote (file));
      return;
    }

  if (user)
    {
      if (group)
	{
	  spec = xmalloc (strlen (user) + 1 + strlen (group) + 1);
	  stpcpy (stpcpy (stpcpy (spec, user), ":"), group);
	  spec_allocated = 1;
	}
      else
	{
	  spec = (char *) user;
	}
    }
  else
    {
      spec = (char *) group;
    }

  switch (changed)
    {
    case CH_SUCCEEDED:
      fmt = (user
	     ? _("changed ownership of %s to %s\n")
	     : _("changed group of %s to %s\n"));
      break;
    case CH_FAILED:
      fmt = (user
	     ? _("failed to change ownership of %s to %s\n")
	     : _("failed to change group of %s to %s\n"));
      break;
    case CH_NO_CHANGE_REQUESTED:
      fmt = (user
	     ? _("ownership of %s retained as %s\n")
	     : _("group of %s retained as %s\n"));
      break;
    default:
      abort ();
    }

  printf (fmt, quote (file), spec);

  if (spec_allocated)
    free (spec);
}

/* Recursively change the ownership of the files in directory DIR to user-id,
   UID, and group-id, GID, according to the options specified by CHOPT.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_owner (const char *dir, uid_t uid, gid_t gid,
		  uid_t old_uid, gid_t old_gid,
		  struct Chown_option const *chopt)
{
  char *name_space, *namep;
  char *path;			/* Full path of each entry to process. */
  unsigned dirlength;		/* Length of `dir' and '\0'. */
  unsigned filelength;		/* Length of each pathname to process. */
  unsigned pathlength;		/* Bytes allocated for `path'. */
  int errors = 0;

  name_space = savedir (dir);
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
      errors |= change_file_owner (0, path, uid, gid, old_uid, old_gid,
				   chopt);
    }
  free (path);
  free (name_space);
  return errors;
}

/* Change the ownership of FILE to user-id, UID, and group-id, GID,
   provided it presently has owner OLD_UID and group OLD_GID.
   Honor the options specified by CHOPT.
   If FILE is a directory and -R is given, recurse.
   Return 0 if successful, 1 if errors occurred. */

int
change_file_owner (int cmdline_arg, const char *file, uid_t uid, gid_t gid,
		   uid_t old_uid, gid_t old_gid,
		   struct Chown_option const *chopt)
{
  struct stat file_stats;
  uid_t new_uid;
  gid_t new_gid;
  int errors = 0;
  int is_symlink;
  int is_directory;

  if (lstat (file, &file_stats))
    {
      if (chopt->force_silent == 0)
	error (0, errno, _("failed to get attributes of %s"), quote (file));
      return 1;
    }

  /* If it's a symlink and we're dereferencing, then use stat
     to get the attributes of the referent.  */
  if (S_ISLNK (file_stats.st_mode))
    {
      if (chopt->dereference == DEREF_ALWAYS
	  && stat (file, &file_stats))
	{
	  if (chopt->force_silent == 0)
	    error (0, errno, _("failed to get attributes of %s"), quote (file));
	  return 1;
	}

      is_symlink = 1;

      /* With -R, don't traverse through symlinks-to-directories.
	 But of course, this will all change with POSIX's new
	 -H, -L, -P options.  */
      is_directory = 0;
    }
  else
    {
      is_symlink = 0;
      is_directory = S_ISDIR (file_stats.st_mode);
    }

  if ((old_uid == (uid_t) -1 || file_stats.st_uid == old_uid)
      && (old_gid == (gid_t) -1 || file_stats.st_gid == old_gid))
    {
      new_uid = (uid == (uid_t) -1 ? file_stats.st_uid : uid);
      new_gid = (gid == (gid_t) -1 ? file_stats.st_gid : gid);
      if (new_uid != file_stats.st_uid || new_gid != file_stats.st_gid)
	{
	  int fail;
	  int symlink_changed = 1;
	  int saved_errno;
	  int called_lchown = 0;

	  if (is_symlink)
	    {
	      if (chopt->dereference == DEREF_NEVER)
		{
		  called_lchown = 1;
		  fail = lchown (file, new_uid, new_gid);

		  /* Ignore the failure if it's due to lack of support (ENOSYS)
		     and this is not a command line argument.  */
		  if (!cmdline_arg && fail && errno == ENOSYS)
		    {
		      fail = 0;
		      symlink_changed = 0;
		    }
		}
	      else if (chopt->dereference == DEREF_ALWAYS)
		{
		  /* Applying chown to a symlink and expecting it to affect
		     the referent is not portable.  So instead, open the
		     file and use fchown on the resulting descriptor.  */
		  int fd = open (file, O_RDONLY | O_NONBLOCK | O_NOCTTY);
		  fail = (fd == -1 ? 1 : fchown (fd, new_uid, new_gid));
		}
	      else
		{
		  /* FIXME */
		  abort ();
		}
	    }
	  else
	    {
	      fail = chown (file, new_uid, new_gid);
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
		error (0, saved_errno, (uid != (uid_t) -1
					? _("changing ownership of %s")
					: _("changing group of %s")),
		       quote (file));
	      errors = 1;
	    }
	  else
	    {
	      /* The change succeeded.  On some systems, the chown function
		 resets the `special' permission bits.  When run by a
		 `privileged' user, this program must ensure that at least
		 the set-uid and set-group ones are still set.  */
	      if (file_stats.st_mode & ~(S_IFMT | S_IRWXUGO)
		  /* If we called lchown above (which means this is a symlink),
		     then skip it.  */
		  && ! called_lchown)
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

  if (chopt->recurse && is_directory)
    errors |= change_dir_owner (file, uid, gid, old_uid, old_gid, chopt);
  return errors;
}
