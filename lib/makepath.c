/* makepath.c -- Ensure that a directory path exists.
   Copyright (C) 1990, 1997, 1998, 1999, 2000, 2002, 2003 Free Software
   Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and Jim Meyering.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
char *alloca ();
#  endif
# endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if STAT_MACROS_BROKEN
# undef S_ISDIR
#endif

#if !defined S_ISDIR && defined S_IFDIR
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_IRWXUGO
# define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#include <errno.h>

#ifndef errno
extern int errno;
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
#endif

#ifndef S_ISUID
# define S_ISUID 04000
#endif
#ifndef S_ISGID
# define S_ISGID 02000
#endif
#ifndef S_ISVTX
# define S_ISVTX 01000
#endif
#ifndef S_IRUSR
# define S_IRUSR 0200
#endif
#ifndef S_IWUSR
# define S_IWUSR 0200
#endif
#ifndef S_IXUSR
# define S_IXUSR 0100
#endif

#ifndef S_IRWXU
# define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif

#define WX_USR (S_IWUSR | S_IXUSR)

/* Include this before libintl.h so we get our definition of PARAMS. */
#include "makepath.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "save-cwd.h"
#include "dirname.h"
#include "error.h"
#include "quote.h"

#define CLEANUP_CWD					\
  do							\
    {							\
      /* We're done operating on basename_dir.		\
	 Restore working directory.  */			\
      if (do_chdir)					\
	{						\
	  if (restore_cwd (&cwd) != 0)			\
	    {						\
	      int _saved_errno = errno;			\
	      error (0, errno,				\
		_("failed to return to initial working directory")); \
	      free_cwd (&cwd);				\
	      errno = _saved_errno;			\
	      return 1;					\
	    }						\
	  free_cwd (&cwd);				\
	}						\
    }							\
  while (0)

#define CLEANUP						\
  do							\
    {							\
      umask (oldmask);					\
      CLEANUP_CWD;					\
    }							\
  while (0)

/* Attempt to create directory DIR (aka DIRPATH) with the specified MODE.
   If CREATED_DIR_P is non-NULL, set *CREATED_DIR_P to non-zero if this
   function creates DIR and to zero otherwise.  Give a diagnostic and
   return non-zero if DIR cannot be created or cannot be determined to
   exist already.  Use DIRPATH in any diagnostic, not DIR.
   Note that if DIR already exists, this function returns zero
   (indicating success) and sets *CREATED_DIR_P to zero.  */

int
make_dir (const char *dir, const char *dirpath, mode_t mode, int *created_dir_p)
{
  int fail = 0;
  int created_dir;

  created_dir = (mkdir (dir, mode) == 0);

  if (!created_dir)
    {
      struct stat stats;
      int saved_errno = errno;

      /* The mkdir and stat calls below may appear to be reversed.
	 They are not.  It is important to call mkdir first and then to
	 call stat (to distinguish the three cases) only if mkdir fails.
	 The alternative to this approach is to `stat' each directory,
	 then to call mkdir if it doesn't exist.  But if some other process
	 were to create the directory between the stat & mkdir, the mkdir
	 would fail with EEXIST.  */

      if (stat (dir, &stats))
	{
	  error (0, saved_errno, _("cannot create directory %s"),
		 quote (dirpath));
	  fail = 1;
	}
      else if (!S_ISDIR (stats.st_mode))
	{
	  error (0, 0, _("%s exists but is not a directory"), quote (dirpath));
	  fail = 1;
	}
      else
	{
	  /* DIR (aka DIRPATH) already exists and is a directory. */
	}
    }

  if (created_dir_p)
    *created_dir_p = created_dir;

  return fail;
}

/* Ensure that the directory ARGPATH exists.

   Create any leading directories that don't already exist, with
   permissions PARENT_MODE.
   If the last element of ARGPATH does not exist, create it as
   a new directory with permissions MODE.
   If OWNER and GROUP are non-negative, use them to set the UID and GID of
   any created directories.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory,
   with the name of the directory that was just made as an argument.
   If PRESERVE_EXISTING is non-zero and ARGPATH is an existing directory,
   then do not attempt to set its permissions and ownership.

   Return 0 if ARGPATH exists as a directory with the proper
   ownership and permissions when done, otherwise 1.  */

int
make_path (const char *argpath,
	   int mode,
	   int parent_mode,
	   uid_t owner,
	   gid_t group,
	   int preserve_existing,
	   const char *verbose_fmt_string)
{
  struct stat stats;
  int retval = 0;

  if (stat (argpath, &stats))
    {
      char *slash;
      int tmp_mode;		/* Initial perms for leading dirs.  */
      int re_protect;		/* Should leading dirs be unwritable? */
      struct ptr_list
      {
	char *dirname_end;
	struct ptr_list *next;
      };
      struct ptr_list *p, *leading_dirs = NULL;
      int do_chdir;		/* Whether to chdir before each mkdir.  */
      struct saved_cwd cwd;
      char *basename_dir;
      char *dirpath;

      /* Temporarily relax umask in case it's overly restrictive.  */
      mode_t oldmask = umask (0);

      /* Make a copy of ARGPATH that we can scribble NULs on.  */
      dirpath = (char *) alloca (strlen (argpath) + 1);
      strcpy (dirpath, argpath);
      strip_trailing_slashes (dirpath);

      /* If leading directories shouldn't be writable or executable,
	 or should have set[ug]id or sticky bits set and we are setting
	 their owners, we need to fix their permissions after making them.  */
      if (((parent_mode & WX_USR) != WX_USR)
	  || ((owner != (uid_t) -1 || group != (gid_t) -1)
	      && (parent_mode & (S_ISUID | S_ISGID | S_ISVTX)) != 0))
	{
	  tmp_mode = S_IRWXU;
	  re_protect = 1;
	}
      else
	{
	  tmp_mode = parent_mode;
	  re_protect = 0;
	}

      /* If we can record the current working directory, we may be able
	 to do the chdir optimization.  */
      do_chdir = !save_cwd (&cwd);

      /* If we've saved the cwd and DIRPATH is an absolute pathname,
	 we must chdir to `/' in order to enable the chdir optimization.
         So if chdir ("/") fails, turn off the optimization.  */
      if (do_chdir && *dirpath == '/' && chdir ("/") < 0)
	do_chdir = 0;

      slash = dirpath;

      /* Skip over leading slashes.  */
      while (*slash == '/')
	slash++;

      while (1)
	{
	  int newly_created_dir;
	  int fail;

	  /* slash points to the leftmost unprocessed component of dirpath.  */
	  basename_dir = slash;

	  slash = strchr (slash, '/');
	  if (slash == NULL)
	    break;

	  /* If we're *not* doing chdir before each mkdir, then we have to refer
	     to the target using the full (multi-component) directory name.  */
	  if (!do_chdir)
	    basename_dir = dirpath;

	  *slash = '\0';
	  fail = make_dir (basename_dir, dirpath, tmp_mode, &newly_created_dir);
	  if (fail)
	    {
	      CLEANUP;
	      return 1;
	    }

	  if (newly_created_dir)
	    {
	      if (verbose_fmt_string)
		error (0, 0, verbose_fmt_string, quote (dirpath));

	      if ((owner != (uid_t) -1 || group != (gid_t) -1)
		  && chown (basename_dir, owner, group)
#if defined AFS && defined EPERM
		  && errno != EPERM
#endif
		  )
		{
		  error (0, errno, _("cannot change owner and/or group of %s"),
			 quote (dirpath));
		  CLEANUP;
		  return 1;
		}

	      if (re_protect)
		{
		  struct ptr_list *new = (struct ptr_list *)
		    alloca (sizeof (struct ptr_list));
		  new->dirname_end = slash;
		  new->next = leading_dirs;
		  leading_dirs = new;
		}
	    }

	  /* If we were able to save the initial working directory,
	     then we can use chdir to change into each directory before
	     creating an entry in that directory.  This avoids making
	     stat and mkdir process O(n^2) file name components.  */
	  if (do_chdir && chdir (basename_dir) < 0)
	    {
	      error (0, errno, _("cannot chdir to directory %s"),
		     quote (dirpath));
	      CLEANUP;
	      return 1;
	    }

	  *slash++ = '/';

	  /* Avoid unnecessary calls to `stat' when given
	     pathnames containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}

      if (!do_chdir)
	basename_dir = dirpath;

      /* Done creating leading directories.  Restore original umask.  */
      umask (oldmask);

      /* We're done making leading directories.
	 Create the final component of the path.  */

      if (make_dir (basename_dir, dirpath, mode, NULL))
	{
	  CLEANUP;
	  return 1;
	}

      if (verbose_fmt_string != NULL)
	error (0, 0, verbose_fmt_string, quote (dirpath));

      if (owner != (uid_t) -1 || group != (gid_t) -1)
	{
	  if (chown (basename_dir, owner, group)
#ifdef AFS
	      && errno != EPERM
#endif
	      )
	    {
	      error (0, errno, _("cannot change owner and/or group of %s"),
		     quote (dirpath));
	      retval = 1;
	    }
	}

      /* The above chown may have turned off some permission bits in MODE.
	 Another reason we may have to use chmod here is that mkdir(2) is
	 required to honor only the file permission bits.  In particular,
	 it need not honor the `special' bits, so if MODE includes any
	 special bits, set them here.  */
      if ((mode & ~S_IRWXUGO)
	  && chmod (basename_dir, mode))
	{
	  error (0, errno, _("cannot change permissions of %s"),
		 quote (dirpath));
	  retval = 1;
	}

      CLEANUP_CWD;

      /* If the mode for leading directories didn't include owner "wx"
	 privileges, we have to reset their protections to the correct
	 value.  */
      for (p = leading_dirs; p != NULL; p = p->next)
	{
	  *(p->dirname_end) = '\0';
	  if (chmod (dirpath, parent_mode))
	    {
	      error (0, errno, "cannot change permissions of %s",
		     quote (dirpath));
	      retval = 1;
	    }
	}
    }
  else
    {
      /* We get here if the entire path already exists.  */

      const char *dirpath = argpath;

      if (!S_ISDIR (stats.st_mode))
	{
	  error (0, 0, _("%s exists but is not a directory"), quote (dirpath));
	  return 1;
	}

      if (!preserve_existing)
	{
	  /* chown must precede chmod because on some systems,
	     chown clears the set[ug]id bits for non-superusers,
	     resulting in incorrect permissions.
	     On System V, users can give away files with chown and then not
	     be able to chmod them.  So don't give files away.  */

	  if ((owner != (uid_t) -1 || group != (gid_t) -1)
	      && chown (dirpath, owner, group)
#ifdef AFS
	      && errno != EPERM
#endif
	      )
	    {
	      error (0, errno, _("cannot change owner and/or group of %s"),
		     quote (dirpath));
	      retval = 1;
	    }
	  if (chmod (dirpath, mode))
	    {
	      error (0, errno, _("cannot change permissions of %s"),
				 quote (dirpath));
	      retval = 1;
	    }
	}
    }

  return retval;
}
