/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
void free ();
#endif

#if defined STDC_HEADERS || defined HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#include <sys/stat.h>

#include <errno.h>

#include "path-concat.h"
#include "xalloc.h"
#include "xgetcwd.h"

#ifndef errno
extern int errno;
#endif

#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif

#if !HAVE_RESOLVEPATH

/* If __PTRDIFF_TYPE__ is
   defined, as with GNU C, use that; that way we don't pollute the
   namespace with <stddef.h>'s symbols.  Otherwise, if <stddef.h> is
   available, include it and use ptrdiff_t.  In traditional C, long is
   the best that we can do.  */

# ifdef __PTRDIFF_TYPE__
#  define PTR_INT_TYPE __PTRDIFF_TYPE__
# else
#  ifdef HAVE_STDDEF_H
#   include <stddef.h>
#   define PTR_INT_TYPE ptrdiff_t
#  else
#   define PTR_INT_TYPE long
#  endif
# endif

# include "pathmax.h"
# include "xreadlink.h"

# ifdef STAT_MACROS_BROKEN
#  undef S_ISLNK
# endif

# ifndef S_ISLNK
#  ifdef S_IFLNK
#   define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#  endif
# endif

#endif /* !HAVE_RESOLVEPATH */

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated path
   separators ('/') or symlinks.  All path components must exist.
   The result is malloc'd.  */

char *
canonicalize_file_name (const char *name)
{
#if HAVE_RESOLVEPATH

  char *resolved, *extra_buf = NULL;
  size_t resolved_size;
  ssize_t resolved_len;

#else /* !HAVE_RESOLVEPATH */

  char *rpath, *dest, *extra_buf = NULL;
  const char *start, *end, *rpath_limit;
  size_t extra_len = 0;
  int num_links = 0;

#endif /* !HAVE_RESOLVEPATH */

  if (name == NULL)
    {
      __set_errno (EINVAL);
      return NULL;
    }

  if (name[0] == '\0')
    {
      __set_errno (ENOENT);
      return NULL;
    }

#if HAVE_RESOLVEPATH

  /* All known hosts with resolvepath (e.g. Solaris 7) don't turn
     relative names into absolute ones, so prepend the working
     directory if the path is not absolute.  */
  if (name[0] != '/')
    {
      char *wd;

      if (!(wd = xgetcwd ()));
	return NULL;

      extra_buf = path_concat (wd, name, NULL);
      if (!extra_buf)
	xalloc_die ();

      name = extra_buf;
      free (wd);
    }

  resolved_size = strlen (name);
  while (1)
    {
      resolved_size = 2 * resolved_size + 1;
      resolved = xmalloc (resolved_size);
      resolved_len = resolvepath (name, resolved, resolved_size);
      if (resolved_len < resolved_size)
	break;
      free (resolved);
    }

  if (resolved_len < 0)
    {
      free (resolved);
      resolved = NULL;
    }

  free (extra_buf);
  return resolved;

#else /* !HAVE_RESOLVEPATH */

  if (name[0] != '/')
    {
      rpath = xgetcwd ();
      if (!rpath)
	return NULL;
      dest = strchr (rpath, '\0');
      if (dest < rpath + PATH_MAX)
	{
	  rpath = xrealloc (rpath, PATH_MAX);
	  rpath_limit = rpath + PATH_MAX;
	}
      else
	{
	  rpath_limit = dest;
	}
    }
  else
    {
      rpath = xmalloc (PATH_MAX);
      rpath_limit = rpath + PATH_MAX;
      rpath[0] = '/';
      dest = rpath + 1;
    }

  for (start = end = name; *start; start = end)
    {
      /* Skip sequence of multiple path-separators.  */
      while (*start == '/')
	++start;

      /* Find end of path component.  */
      for (end = start; *end && *end != '/'; ++end)
	/* Nothing.  */;

      if (end - start == 0)
	break;
      else if (end - start == 1 && start[0] == '.')
	/* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
	{
	  /* Back up to previous component, ignore if at root already.  */
	  if (dest > rpath + 1)
	    while ((--dest)[-1] != '/');
	}
      else
	{
	  struct stat st;

	  if (dest[-1] != '/')
	    *dest++ = '/';

	  if (dest + (end - start) >= rpath_limit)
	    {
	      PTR_INT_TYPE dest_offset = dest - rpath;
	      size_t new_size = rpath_limit - rpath;

	      if (end - start + 1 > PATH_MAX)
		new_size += end - start + 1;
	      else
		new_size += PATH_MAX;
	      rpath = (char *) xrealloc (rpath, new_size);
	      rpath_limit = rpath + new_size;

	      dest = rpath + dest_offset;
	    }

	  dest = memcpy (dest, start, end - start);
	  dest += end - start;
	  *dest = '\0';

	  if (lstat (rpath, &st) < 0)
	    goto error;

# ifdef S_ISLNK
	  if (S_ISLNK (st.st_mode))
	    {
	      char *buf;
	      size_t n, len;

#  ifdef MAXSYMLINKS
	      if (++num_links > MAXSYMLINKS)
		{
		  __set_errno (ELOOP);
		  goto error;
		}
#  endif /* MAXSYMLINKS */

	      buf = xreadlink (rpath);
	      if (!buf)
		goto error;

	      n = strlen (buf);
	      len = strlen (end);

	      if (!extra_len)
		{
		  extra_len =
		    ((n + len + 1) > PATH_MAX) ? (n + len + 1) : PATH_MAX;
		  extra_buf = xmalloc (extra_len);
		}
	      else if ((n + len + 1) > extra_len)
		{
		  extra_len = n + len + 1;
		  extra_buf = xrealloc (extra_buf, extra_len);
		}

	      /* Careful here, end may be a pointer into extra_buf... */
	      memmove (&extra_buf[n], end, len + 1);
	      name = end = memcpy (extra_buf, buf, n);

	      if (buf[0] == '/')
		dest = rpath + 1;	/* It's an absolute symlink */
	      else
		/* Back up to previous component, ignore if at root already: */
		if (dest > rpath + 1)
		  while ((--dest)[-1] != '/');

	      free (buf);
	    }
# endif /* S_ISLNK */
	}
    }
  if (dest > rpath + 1 && dest[-1] == '/')
    --dest;
  *dest = '\0';

  free (extra_buf);
  return rpath;

error:
  free (extra_buf);
  free (rpath);
  return NULL;
#endif /* !HAVE_RESOLVEPATH */
}
