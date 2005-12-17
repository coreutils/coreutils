/* acl.c - access control lists

   Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Paul Eggert and Andreas Gruenbacher.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef S_ISLNK
# define S_ISLNK(Mode) 0
#endif

#ifdef HAVE_ACL_LIBACL_H
# include <acl/libacl.h>
#endif

#include "acl.h"
#include "error.h"
#include "quote.h"

#include <errno.h>
#ifndef ENOSYS
# define ENOSYS (-1)
#endif
#ifndef ENOTSUP
# define ENOTSUP (-1)
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif

#ifndef HAVE_FCHMOD
# define HAVE_FCHMOD false
# define fchmod(fd, mode) (-1)
#endif

/* POSIX 1003.1e (draft 17) */
#ifndef HAVE_ACL_GET_FD
# define HAVE_ACL_GET_FD false
# define acl_get_fd(fd) (NULL)
#endif

/* POSIX 1003.1e (draft 17) */
#ifndef HAVE_ACL_SET_FD
# define HAVE_ACL_SET_FD false
# define acl_set_fd(fd, acl) (-1)
#endif

/* Linux-specific */
#ifndef HAVE_ACL_EXTENDED_FILE
# define HAVE_ACL_EXTENDED_FILE false
# define acl_extended_file(name) (-1)
#endif

/* Linux-specific */
#ifndef HAVE_ACL_FROM_MODE
# define HAVE_ACL_FROM_MODE false
# define acl_from_mode(mode) (NULL)
#endif

/* We detect presence of POSIX 1003.1e (draft 17 -- abandoned) support
   by checking for HAVE_ACL_GET_FILE, HAVE_ACL_SET_FILE, and HAVE_ACL_FREE.
   Systems that have acl_get_file, acl_set_file, and acl_free must also
   have acl_to_text, acl_from_text, and acl_delete_def_file (all defined
   in the draft); systems that don't would hit #error statements here.  */

#if USE_ACL && HAVE_ACL_GET_FILE && !HAVE_ACL_ENTRIES
# ifndef HAVE_ACL_TO_TEXT
#  error Must have acl_to_text (see POSIX 1003.1e draft 17).
# endif

/* Return the number of entries in ACL. Linux implements acl_entries
   as a more efficient extension than using this workaround.  */

static int
acl_entries (acl_t acl)
{
  char *text = acl_to_text (acl, NULL), *t;
  int entries;
  if (text == NULL)
    return -1;
  for (entries = 0, t = text; ; t++, entries++) {
    t = strchr (t, '\n');
    if (t == NULL)
      break;
  }
  acl_free (text);
  return entries;
}
#endif

/* If DESC is a valid file descriptor use fchmod to change the
   file's mode to MODE on systems that have fchown. On systems
   that don't have fchown and if DESC is invalid, use chown on
   NAME instead.  */

int
chmod_or_fchmod (const char *name, int desc, mode_t mode)
{
  if (HAVE_FCHMOD && desc != -1)
    return fchmod (desc, mode);
  else
    return chmod (name, mode);
}

/* Return 1 if NAME has a nontrivial access control list, 0 if
   NAME only has no or a base access control list, and -1 on
   error.  SB must be set to the stat buffer of FILE.  */

int
file_has_acl (char const *name, struct stat const *sb)
{
#if USE_ACL && HAVE_ACL && defined GETACLCNT
  /* This implementation should work on recent-enough versions of HP-UX,
     Solaris, and Unixware.  */

# ifndef MIN_ACL_ENTRIES
#  define MIN_ACL_ENTRIES 4
# endif

  if (! S_ISLNK (sb->st_mode))
    {
      int n = acl (name, GETACLCNT, 0, NULL);
      return n < 0 ? (errno == ENOSYS ? 0 : -1) : (MIN_ACL_ENTRIES < n);
    }
#elif USE_ACL && HAVE_ACL_GET_FILE && HAVE_ACL_FREE
  /* POSIX 1003.1e (draft 17 -- abandoned) specific version.  */

  if (! S_ISLNK (sb->st_mode))
    {
      int ret;

      if (HAVE_ACL_EXTENDED_FILE)
	ret = acl_extended_file (name);
      else
	{
	  acl_t acl = acl_get_file (name, ACL_TYPE_ACCESS);
	  if (acl)
	    {
	      ret = (3 < acl_entries (acl));
	      acl_free (acl);
	      if (ret == 0 && S_ISDIR (sb->st_mode))
		{
		  acl = acl_get_file (name, ACL_TYPE_DEFAULT);
		  if (acl)
		    {
		      ret = (0 < acl_entries (acl));
		      acl_free (acl);
		    }
		  else
		    ret = -1;
		}
	    }
	  else
	    ret = -1;
	}
      if (ret < 0)
	return (errno == ENOSYS || errno == ENOTSUP) ? 0 : -1;
      return ret;
    }
#endif

  /* FIXME: Add support for AIX, Irix, and Tru64.  Please see Samba's
     source/lib/sysacls.c file for fix-related ideas.  */

  return 0;
}

/* Copy access control lists from one file to another. If SOURCE_DESC is
   a valid file descriptor, use file descriptor operations, else use
   filename based operations on SRC_NAME. Likewise for DEST_DESC and
   DEST_NAME.
   If access control lists are not available, fchmod the target file to
   MODE.  Also sets the non-permission bits of the destination file
   (S_ISUID, S_ISGID, S_ISVTX) to those from MODE if any are set.
   System call return value semantics.  */

int
copy_acl (const char *src_name, int source_desc, const char *dst_name,
	  int dest_desc, mode_t mode)
{
  int ret;

#if USE_ACL && HAVE_ACL_GET_FILE && HAVE_ACL_SET_FILE && HAVE_ACL_FREE
  /* POSIX 1003.1e (draft 17 -- abandoned) specific version.  */

  acl_t acl;
  if (HAVE_ACL_GET_FD && source_desc != -1)
    acl = acl_get_fd (source_desc);
  else
    acl = acl_get_file (src_name, ACL_TYPE_ACCESS);
  if (acl == NULL)
    {
      if (errno == ENOSYS || errno == ENOTSUP)
	return set_acl (dst_name, dest_desc, mode);
      else
        {
	  error (0, errno, "%s", quote (src_name));
	  return -1;
	}
    }

  if (HAVE_ACL_SET_FD && dest_desc != -1)
    ret = acl_set_fd (dest_desc, acl);
  else
    ret = acl_set_file (dst_name, ACL_TYPE_ACCESS, acl);
  if (ret != 0)
    {
      int saved_errno = errno;

      if (errno == ENOSYS || errno == ENOTSUP)
        {
	  int n = acl_entries (acl);

	  acl_free (acl);
	  if (n == 3)
	    {
	      if (chmod_or_fchmod (dst_name, dest_desc, mode) != 0)
		saved_errno = errno;
	      else
		return 0;
	    }
	  else
	    chmod_or_fchmod (dst_name, dest_desc, mode);
	}
      else
	{
	  acl_free (acl);
	  chmod_or_fchmod (dst_name, dest_desc, mode);
	}
      error (0, saved_errno, _("preserving permissions for %s"),
	     quote (dst_name));
      return -1;
    }
  else
    acl_free (acl);

  if (mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* We did not call chmod so far, so the special bits have not yet
         been set.  */

      if (chmod_or_fchmod (dst_name, dest_desc, mode) != 0)
	{
	  error (0, errno, _("preserving permissions for %s"),
		 quote (dst_name));
	  return -1;
	}
    }

  if (S_ISDIR (mode))
    {
      acl = acl_get_file (src_name, ACL_TYPE_DEFAULT);
      if (acl == NULL)
	{
	  error (0, errno, "%s", quote (src_name));
	  return -1;
	}

      if (acl_set_file (dst_name, ACL_TYPE_DEFAULT, acl))
	{
	  error (0, errno, _("preserving permissions for %s"),
		 quote (dst_name));
	  acl_free (acl);
	  return -1;
	}
      else
        acl_free (acl);
    }
  return 0;
#else
  ret = chmod_or_fchmod (dst_name, dest_desc, mode);
  if (ret != 0)
    error (0, errno, _("preserving permissions for %s"), quote (dst_name));
  return ret;
#endif
}

/* Set the access control lists of a file. If DESC is a valid file
   descriptor, use file descriptor operations where available, else use
   filename based operations on NAME.  If access control lists are not
   available, fchmod the target file to MODE.  Also sets the
   non-permission bits of the destination file (S_ISUID, S_ISGID, S_ISVTX)
   to those from MODE if any are set.  System call return value
   semantics.  */

int
set_acl (char const *name, int desc, mode_t mode)
{
#if USE_ACL && HAVE_ACL_SET_FILE && HAVE_ACL_FREE
  /* POSIX 1003.1e draft 17 (abandoned) specific version.  */

  /* We must also have have_acl_from_text and acl_delete_def_file.
     (acl_delete_def_file could be emulated with acl_init followed
      by acl_set_file, but acl_set_file with an empty acl is
      unspecified.)  */

# ifndef HAVE_ACL_FROM_TEXT
#  error Must have acl_from_text (see POSIX 1003.1e draft 17).
# endif
# ifndef HAVE_ACL_DELETE_DEF_FILE
#  error Must have acl_delete_def_file (see POSIX 1003.1e draft 17).
# endif

  acl_t acl;
  int ret;

  if (HAVE_ACL_FROM_MODE)
    {
      acl = acl_from_mode (mode);
      if (!acl)
	{
	  error (0, errno, "%s", quote (name));
	  return -1;
	}
    }
  else
    {
      char acl_text[] = "u::---,g::---,o::---";

      if (mode & S_IRUSR) acl_text[ 3] = 'r';
      if (mode & S_IWUSR) acl_text[ 4] = 'w';
      if (mode & S_IXUSR) acl_text[ 5] = 'x';
      if (mode & S_IRGRP) acl_text[10] = 'r';
      if (mode & S_IWGRP) acl_text[11] = 'w';
      if (mode & S_IXGRP) acl_text[12] = 'x';
      if (mode & S_IROTH) acl_text[17] = 'r';
      if (mode & S_IWOTH) acl_text[18] = 'w';
      if (mode & S_IXOTH) acl_text[19] = 'x';

      acl = acl_from_text (acl_text);
      if (!acl)
	{
	  error (0, errno, "%s", quote (name));
	  return -1;
	}
    }
  if (HAVE_ACL_SET_FD && desc != -1)
    ret = acl_set_fd (desc, acl);
  else
    ret = acl_set_file (name, ACL_TYPE_ACCESS, acl);
  if (ret != 0)
    {
      int saved_errno = errno;
      acl_free (acl);

      if (errno == ENOTSUP || errno == ENOSYS)
	{
	  if (chmod_or_fchmod (name, desc, mode) != 0)
	    saved_errno = errno;
	  else
	    return 0;
	}
      error (0, saved_errno, _("setting permissions for %s"), quote (name));
      return -1;
    }
  else
    acl_free (acl);

  if (S_ISDIR (mode) && acl_delete_def_file (name))
    {
      error (0, errno, _("setting permissions for %s"), quote (name));
      return -1;
    }

  if (mode & (S_ISUID | S_ISGID | S_ISVTX))
    {
      /* We did not call chmod so far, so the special bits have not yet
         been set.  */

      if (chmod_or_fchmod (name, desc, mode))
	{
	  error (0, errno, _("preserving permissions for %s"), quote (name));
	  return -1;
	}
    }
  return 0;
#else
   int ret = chmod_or_fchmod (name, desc, mode);
   if (ret)
     error (0, errno, _("setting permissions for %s"), quote (name));
   return ret;
#endif
}
