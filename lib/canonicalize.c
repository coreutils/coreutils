/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2005 Free Software Foundation, Inc.

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
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "canonicalize.h"

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

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <errno.h>
#include <stddef.h>

#include "cycle-check.h"
#include "filenamecat.h"
#include "stat-macros.h"
#include "xalloc.h"
#include "xgetcwd.h"

#ifndef __set_errno
# define __set_errno(Val) errno = (Val)
#endif

#include "pathmax.h"
#include "xreadlink.h"

#if !HAVE_CANONICALIZE_FILE_NAME
/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated file name
   separators ('/') or symlinks.  All components must exist.
   The result is malloc'd.  */

char *
canonicalize_file_name (const char *name)
{
# if HAVE_RESOLVEPATH

  char *resolved, *extra_buf = NULL;
  size_t resolved_size;
  ssize_t resolved_len;

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

  /* All known hosts with resolvepath (e.g. Solaris 7) don't turn
     relative names into absolute ones, so prepend the working
     directory if the file name is not absolute.  */
  if (name[0] != '/')
    {
      char *wd;

      if (!(wd = xgetcwd ()))
	return NULL;

      extra_buf = file_name_concat (wd, name, NULL);
      name = extra_buf;
      free (wd);
    }

  resolved_size = strlen (name);
  while (1)
    {
      resolved_size = 2 * resolved_size + 1;
      resolved = xmalloc (resolved_size);
      resolved_len = resolvepath (name, resolved, resolved_size);
      if (resolved_len < 0)
	{
	  free (resolved);
	  free (extra_buf);
	  return NULL;
	}
      if (resolved_len < resolved_size)
	break;
      free (resolved);
    }

  free (extra_buf);

  /* NUL-terminate the resulting name.  */
  resolved[resolved_len] = '\0';

  return resolved;

# else

  return canonicalize_filename_mode (name, CAN_EXISTING);

# endif /* !HAVE_RESOLVEPATH */
}
#endif /* !HAVE_CANONICALIZE_FILE_NAME */

/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any `.', `..' components nor any repeated file name
   separators ('/') or symlinks.  Whether components must exist
   or not depends on canonicalize mode.  The result is malloc'd.  */

char *
canonicalize_filename_mode (const char *name, canonicalize_mode_t can_mode)
{
  char *rname, *dest, *extra_buf = NULL;
  char const *start;
  char const *end;
  char const *rname_limit;
  size_t extra_len = 0;
  struct cycle_check_state cycle_state;

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

  if (name[0] != '/')
    {
      rname = xgetcwd ();
      if (!rname)
	return NULL;
      dest = strchr (rname, '\0');
      if (dest - rname < PATH_MAX)
	{
	  char *p = xrealloc (rname, PATH_MAX);
	  dest = p + (dest - rname);
	  rname = p;
	  rname_limit = rname + PATH_MAX;
	}
      else
	{
	  rname_limit = dest;
	}
    }
  else
    {
      rname = xmalloc (PATH_MAX);
      rname_limit = rname + PATH_MAX;
      rname[0] = '/';
      dest = rname + 1;
    }

  cycle_check_init (&cycle_state);
  for (start = end = name; *start; start = end)
    {
      /* Skip sequence of multiple file name separators.  */
      while (*start == '/')
	++start;

      /* Find end of component.  */
      for (end = start; *end && *end != '/'; ++end)
	/* Nothing.  */;

      if (end - start == 0)
	break;
      else if (end - start == 1 && start[0] == '.')
	/* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
	{
	  /* Back up to previous component, ignore if at root already.  */
	  if (dest > rname + 1)
	    while ((--dest)[-1] != '/');
	}
      else
	{
	  struct stat st;

	  if (dest[-1] != '/')
	    *dest++ = '/';

	  if (dest + (end - start) >= rname_limit)
	    {
	      ptrdiff_t dest_offset = dest - rname;
	      size_t new_size = rname_limit - rname;

	      if (end - start + 1 > PATH_MAX)
		new_size += end - start + 1;
	      else
		new_size += PATH_MAX;
	      rname = xrealloc (rname, new_size);
	      rname_limit = rname + new_size;

	      dest = rname + dest_offset;
	    }

	  dest = memcpy (dest, start, end - start);
	  dest += end - start;
	  *dest = '\0';

	  if (lstat (rname, &st) != 0)
	    {
	      if (can_mode == CAN_EXISTING)
		goto error;
	      if (can_mode == CAN_ALL_BUT_LAST && *end)
		goto error;
	      st.st_mode = 0;
	    }

	  if (S_ISLNK (st.st_mode))
	    {
	      char *buf;
	      size_t n, len;

	      if (cycle_check (&cycle_state, &st))
		{
		  __set_errno (ELOOP);
		  if (can_mode == CAN_MISSING)
		    continue;
		  else
		    goto error;
		}

	      buf = xreadlink (rname, st.st_size);
	      if (!buf)
		{
		  if (can_mode == CAN_MISSING)
		    continue;
		  else
		    goto error;
		}

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
		dest = rname + 1;	/* It's an absolute symlink */
	      else
		/* Back up to previous component, ignore if at root already: */
		if (dest > rname + 1)
		  while ((--dest)[-1] != '/');

	      free (buf);
	    }
	  else
	    {
	      if (!S_ISDIR (st.st_mode) && *end && (can_mode != CAN_MISSING))
		{
		  errno = ENOTDIR;
		  goto error;
		}
	    }
	}
    }
  if (dest > rname + 1 && dest[-1] == '/')
    --dest;
  *dest = '\0';

  free (extra_buf);
  return rname;

error:
  free (extra_buf);
  free (rname);
  return NULL;
}
