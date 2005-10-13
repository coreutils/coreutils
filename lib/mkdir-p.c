/* mkdir-p.c -- Ensure that a directory and its parents exist.

   Copyright (C) 1990, 1997, 1998, 1999, 2000, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "mkdir-p.h"

#include <alloca.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "save-cwd.h"
#include "dirname.h"
#include "error.h"
#include "quote.h"
#include "stat-macros.h"

#define WX_USR (S_IWUSR | S_IXUSR)

/* Ensure that the directory ARG exists.

   Create any leading directories that don't already exist, with
   permissions PARENT_MODE.
   If the last element of ARG does not exist, create it as
   a new directory with permissions MODE.
   If OWNER and GROUP are non-negative, use them to set the UID and GID of
   any created directories.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory,
   with the name of the directory that was just made as an argument.
   If PRESERVE_EXISTING is true and ARG is an existing directory,
   then do not attempt to set its permissions and ownership.

   Set *CWD_ERRNO to a (nonzero) error number if this
   function has changed the current working directory and is unable to
   restore it to its initial state.  Do not change
   *CWD_ERRNO otherwise.

   Return true iff ARG exists as a directory with the proper ownership
   and permissions when done.  Note that this function returns true
   even when it fails to return to the initial working directory.  */

bool
make_dir_parents (char const *arg,
		  mode_t mode,
		  mode_t parent_mode,
		  uid_t owner,
		  gid_t group,
		  bool preserve_existing,
		  char const *verbose_fmt_string,
		  int *cwd_errno)
{
  struct stat stats;
  bool retval = true;
  bool do_chdir = false;	/* Whether to chdir before each mkdir.  */
  struct saved_cwd cwd;
  bool cwd_problem = false;
  char const *fixup_permissions_dir = NULL;
  char const *full_dir = arg;

  struct ptr_list
  {
    char *dirname_end;
    struct ptr_list *next;
  };
  struct ptr_list *leading_dirs = NULL;

  if (stat (arg, &stats) == 0)
    {
      if (! S_ISDIR (stats.st_mode))
	{
	  error (0, 0, _("%s exists but is not a directory"), quote (arg));
	  return false;
	}

      if (!preserve_existing)
	fixup_permissions_dir = arg;
    }
  else if (errno != ENOENT || !*arg)
    {
      error (0, errno, "%s", quote (arg));
      return false;
    }
  else
    {
      char *slash;
      mode_t tmp_mode;		/* Initial perms for leading dirs.  */
      bool re_protect;		/* Should leading dirs be unwritable? */
      char *basename_dir;
      char *dir;

      /* Temporarily relax umask in case it's overly restrictive.  */
      mode_t oldmask = umask (0);

      /* Make a copy of ARG that we can scribble NULs on.  */
      dir = (char *) alloca (strlen (arg) + 1);
      strcpy (dir, arg);
      strip_trailing_slashes (dir);
      full_dir = dir;

      /* If leading directories shouldn't be writable or executable,
	 or should have set[ug]id or sticky bits set and we are setting
	 their owners, we need to fix their permissions after making them.  */
      if (((parent_mode & WX_USR) != WX_USR)
	  || ((owner != (uid_t) -1 || group != (gid_t) -1)
	      && (parent_mode & (S_ISUID | S_ISGID | S_ISVTX)) != 0))
	{
	  tmp_mode = S_IRWXU;
	  re_protect = true;
	}
      else
	{
	  tmp_mode = parent_mode;
	  re_protect = false;
	}

      /* If we can record the current working directory, we may be able
	 to do the chdir optimization.  */
      do_chdir = (save_cwd (&cwd) == 0);

      /* If we've saved the cwd and DIR is an absolute file name,
	 we must chdir to `/' in order to enable the chdir optimization.
         So if chdir ("/") fails, turn off the optimization.  */
      if (do_chdir && dir[0] == '/')
	{
	  /* POSIX says "//" might be special, so chdir to "//" if the
	     file name starts with exactly two slashes.  */
	  char const *root = "//" + (dir[1] != '/' || dir[2] == '/');
	  if (chdir (root) != 0)
	    {
	      free_cwd (&cwd);
	      do_chdir = false;
	    }
	}

      slash = dir;

      /* Skip over leading slashes.  */
      while (*slash == '/')
	slash++;

      while (true)
	{
	  bool dir_known_to_exist;
	  int mkdir_errno;

	  /* slash points to the leftmost unprocessed component of dir.  */
	  basename_dir = slash;

	  slash = strchr (slash, '/');
	  if (slash == NULL)
	    break;

	  /* If we're *not* doing chdir before each mkdir, then we have to refer
	     to the target using the full (multi-component) directory name.  */
	  if (!do_chdir)
	    basename_dir = dir;

	  *slash = '\0';
	  dir_known_to_exist = (mkdir (basename_dir, tmp_mode) == 0);
	  mkdir_errno = errno;

	  if (dir_known_to_exist)
	    {
	      if (verbose_fmt_string)
		error (0, 0, verbose_fmt_string, quote (dir));

	      if ((owner != (uid_t) -1 || group != (gid_t) -1)
		  && chown (basename_dir, owner, group)
#if defined AFS && defined EPERM
		  && errno != EPERM
#endif
		  )
		{
		  error (0, errno, _("cannot change owner and/or group of %s"),
			 quote (dir));
		  retval = false;
		  break;
		}

	      if (re_protect)
		{
		  struct ptr_list *new = (struct ptr_list *)
		    alloca (sizeof *new);
		  new->dirname_end = slash;
		  new->next = leading_dirs;
		  leading_dirs = new;
		}
	    }

	  /* If we were able to save the initial working directory,
	     then we can use chdir to change into each directory before
	     creating an entry in that directory.  This avoids making
	     mkdir process O(n^2) file name components.  */
	  if (do_chdir)
	    {
	      if (chdir (basename_dir) == 0)
		dir_known_to_exist = true;
	      else if (dir_known_to_exist)
		{
		  error (0, errno, _("cannot chdir to directory %s"),
			 quote (dir));
		  retval = false;
		  break;
		}
	    }
	  else if (!dir_known_to_exist)
	    dir_known_to_exist = (stat (basename_dir, &stats) == 0
				  && S_ISDIR (stats.st_mode));

	  if (!dir_known_to_exist)
	    {
	      error (0, mkdir_errno, _("cannot create directory %s"),
		     quote (dir));
	      retval = false;
	      break;
	    }

	  *slash++ = '/';

	  /* Avoid unnecessary calls to mkdir when given
	     file names containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}

      if (!do_chdir)
	basename_dir = dir;

      /* Done creating leading directories.  Restore original umask.  */
      umask (oldmask);

      /* We're done making leading directories.
	 Create the final component of the file name.  */
      if (retval)
	{
	  if (mkdir (basename_dir, mode) != 0)
	    {
	      error (0, errno, _("cannot create directory %s"), quote (dir));
	      retval = false;
	    }
	  else
	    {
	      if (verbose_fmt_string)
		error (0, 0, verbose_fmt_string, quote (dir));
	      fixup_permissions_dir = basename_dir;
	    }
	}
    }

  if (fixup_permissions_dir)
    {
      /* chown must precede chmod because on some systems,
	 chown clears the set[ug]id bits for non-superusers,
	 resulting in incorrect permissions.
	 On System V, users can give away files with chown and then not
	 be able to chmod them.  So don't give files away.  */

      if (owner != (uid_t) -1 || group != (gid_t) -1)
	{
	  if (chown (fixup_permissions_dir, owner, group) != 0
#ifdef AFS
	      && errno != EPERM
#endif
	      )
	    {
	      error (0, errno, _("cannot change owner and/or group of %s"),
		     quote (full_dir));
	      retval = false;
	    }
	}

      /* The above chown may have turned off some permission bits in MODE.
	 Another reason we may have to use chmod here is that mkdir(2) is
	 required to honor only the file permission bits.  In particular,
	 it need not honor the `special' bits, so if MODE includes any
	 special bits, set them here.  */
      if ((mode & ~S_IRWXUGO) && chmod (fixup_permissions_dir, mode) != 0)
	{
	  error (0, errno, _("cannot change permissions of %s"),
		 quote (full_dir));
	  retval = false;
	}
    }

  if (do_chdir)
    {
      if (restore_cwd (&cwd) != 0)
	{
	  *cwd_errno = errno;
	  cwd_problem = true;
	}
      free_cwd (&cwd);
    }

  /* If the mode for leading directories didn't include owner "wx"
     privileges, reset their protections to the correct value.  */
  for (; leading_dirs != NULL; leading_dirs = leading_dirs->next)
    {
      leading_dirs->dirname_end[0] = '\0';
      if ((cwd_problem && *full_dir != '/')
	  || chmod (full_dir, parent_mode) != 0)
	{
	  error (0, (cwd_problem ? 0 : errno),
		 _("cannot change permissions of %s"), quote (full_dir));
	  retval = false;
	}
    }

  return retval;
}
