/* chown-core.c -- core functions for changing ownership.
   Copyright (C) 2000, 2002, 2003, 2004 Free Software Foundation.

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
#include "xfts.h"
#include "lchown.h"
#include "quote.h"
#include "root-dev-ino.h"
#include "chown-core.h"

/* The number of decimal digits required to represent the largest value of
   type `unsigned int'.  This is enough for an 8-byte unsigned int type.  */
#define UINT_MAX_DECIMAL_DIGITS 20

#ifndef _POSIX_VERSION
struct group *getgrnam ();
struct group *getgrgid ();
#endif

extern void
chopt_init (struct Chown_option *chopt)
{
  chopt->verbosity = V_off;
  chopt->root_dev_ino = NULL;
  chopt->affect_symlink_referent = true;
  chopt->recurse = false;
  chopt->force_silent = false;
  chopt->user_name = 0;
  chopt->group_name = 0;
}

extern void
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

extern char *
gid_to_name (gid_t gid)
{
  struct group *grp = getgrgid (gid);
  return grp ? xstrdup (grp->gr_name) : uint_to_string (gid);
}

/* Convert the numeric user-id, UID, to a string stored in xmalloc'd memory,
   and return it.  If there's no corresponding user name, use the decimal
   representation of the ID.  */

extern char *
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

/* Change the owner and/or group of the file specified by FTS and ENT
   to UID and/or GID as appropriate.
   FIXME: describe old_uid and old_gid.
   CHOPT specifies additional options.
   Return 0 if successful, -1 if errors occurred.  */
static int
change_file_owner (FTS *fts, FTSENT *ent,
		   uid_t uid, gid_t gid,
		   uid_t old_uid, gid_t old_gid,
		   struct Chown_option const *chopt)
{
  char const *file_full_name = ent->fts_path;
  char const *file = ent->fts_accpath;
  struct stat const *file_stats IF_LINT (= NULL);
  struct stat stat_buf;
  int errors = 0;
  bool do_chown;
  bool symlink_changed = true;

  switch (ent->fts_info)
    {
    case FTS_D:
      if (chopt->recurse)
	return 0;
      break;

    case FTS_DP:
      if (! chopt->recurse)
	return 0;
      break;

    case FTS_NS:
      error (0, ent->fts_errno, _("cannot access %s"), quote (file_full_name));
      errors = -1;
      break;

    case FTS_ERR:
      error (0, ent->fts_errno, _("%s"), quote (file_full_name));
      errors = -1;
      break;

    case FTS_DNR:
      error (0, ent->fts_errno, _("cannot read directory %s"),
	     quote (file_full_name));
      errors = -1;
      break;

    default:
      break;
    }

  if (errors)
    do_chown = false;
  else if (old_uid == (uid_t) -1 && old_gid == (gid_t) -1
	   && chopt->verbosity == V_off && ! chopt->root_dev_ino)
    do_chown = true;
  else
    {
      file_stats = ent->fts_statp;

      /* If this is a symlink and we're dereferencing them,
	 stat it to get the permissions of the referent.  */
      if (S_ISLNK (file_stats->st_mode) && chopt->affect_symlink_referent)
	{
	  if (stat (file, &stat_buf) != 0)
	    {
	      error (0, errno, _("cannot dereference %s"),
		     quote (file_full_name));
	      errors = -1;
	    }

	  file_stats = &stat_buf;
	}

      do_chown = (!errors
		  && (old_uid == (uid_t) -1 || old_uid == file_stats->st_uid)
		  && (old_gid == (gid_t) -1 || old_gid == file_stats->st_gid));
    }

  if (do_chown && ROOT_DEV_INO_CHECK (chopt->root_dev_ino, file_stats))
    {
      ROOT_DEV_INO_WARN (file_full_name);
      errors = -1;
      do_chown = false;
    }

  if (do_chown)
    {
      if (chopt->affect_symlink_referent)
	{
	  /* Applying chown to a symlink and expecting it to affect
	     the referent is not portable, but here we may be using a
	     wrapper that tries to correct for unconforming chown.  */
	  errors = chown (file, uid, gid);
	}
      else
	{
	  errors = lchown (file, uid, gid);

	  /* Ignore any error due to lack of support; POSIX requires
	     this behavior for top-level symbolic links with -h, and
	     implies that it's required for all symbolic links.  */
	  if (errors && errno == EOPNOTSUPP)
	    {
	      errors = 0;
	      symlink_changed = false;
	    }
	}

      /* On some systems (e.g., Linux-2.4.x),
	 the chown function resets the `special' permission bits.
	 Do *not* restore those bits;  doing so would open a window in
	 which a malicious user, M, could subvert a chown command run
	 by some other user and operating on files in a directory
	 where M has write access.  */

      if (errors && ! chopt->force_silent)
	error (0, errno, (uid != (uid_t) -1
			  ? _("changing ownership of %s")
			  : _("changing group of %s")),
	       quote (file_full_name));
    }

  if (chopt->verbosity != V_off)
    {
      bool changed =
	(do_chown && !errors && symlink_changed
	 && ! ((uid == (uid_t) -1 || uid == file_stats->st_uid)
	       && (gid == (gid_t) -1 || gid == file_stats->st_gid)));

      if (changed || chopt->verbosity == V_high)
	{
	  enum Change_status ch_status =
	    (errors ? CH_FAILED
	     : !symlink_changed ? CH_NOT_APPLIED
	     : !changed ? CH_NO_CHANGE_REQUESTED
	     : CH_SUCCEEDED);
	  describe_change (file_full_name, ch_status,
			   chopt->user_name, chopt->group_name);
	}
    }

  if ( ! chopt->recurse)
    fts_set (fts, ent, FTS_SKIP);

  return errors;
}

/* Change the owner and/or group of the specified FILES.
   BIT_FLAGS specifies how to treat each symlink-to-directory
   that is encountered during a recursive traversal.
   CHOPT specifies additional options.
   If UID is not -1, then change the owner id of each file to UID.
   If GID is not -1, then change the group id of each file to GID.
   If REQUIRED_UID and/or REQUIRED_GID is not -1, then change only
   files with user ID and group ID that match the non-(-1) value(s).
   Return nonzero upon error, zero otherwise.  */
extern int
chown_files (char **files, int bit_flags,
	     uid_t uid, gid_t gid,
	     uid_t required_uid, gid_t required_gid,
	     struct Chown_option const *chopt)
{
  int fail = 0;

  /* Use lstat and stat only if they're needed.  */
  int stat_flags = ((required_uid != (uid_t) -1 || required_gid != (gid_t) -1
		     || chopt->verbosity != V_off || chopt->root_dev_ino)
		    ? 0
		    : FTS_NOSTAT);

  FTS *fts = xfts_open (files, bit_flags | stat_flags, NULL);

  while (1)
    {
      FTSENT *ent;

      ent = fts_read (fts);
      if (ent == NULL)
	{
	  if (errno != 0)
	    {
	      /* FIXME: try to give a better message  */
	      error (0, errno, _("fts_read failed"));
	      fail = 1;
	    }
	  break;
	}

      fail |= change_file_owner (fts, ent, uid, gid,
				 required_uid, required_gid, chopt);
    }

  /* Ignore failure, since the only way it can do so is in failing to
     return to the original directory, and since we're about to exit,
     that doesn't matter.  */
  fts_close (fts);

  return fail;
}
