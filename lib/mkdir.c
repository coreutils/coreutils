/* On some systems, mkdir ("foo/", 0700) fails because of the trailing
   slash.  On those systems, this wrapper removes the trailing slash.
   Copyright (C) 2001 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

/* Disable the definition of mkdir to rpl_mkdir (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_mkdir on
   most systems.  */
#undef mkdir

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include "dirname.h"
#include "xalloc.h"

#ifndef HAVE_DECL_FREE
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_FREE
void free ();
#endif

/* This function is required at least for NetBSD 1.5.2.  */

int
rpl_mkdir (char const *dir, mode_t mode)
{
  int ret_val;
  char *tmp_dir;
  size_t len = strlen (dir);

  if (len && dir[len - 1] == '/')
    {
      tmp_dir = xstrdup (dir);
      strip_trailing_slashes (tmp_dir);
    }
  else
    {
      tmp_dir = (char *) dir;
    }

  ret_val = mkdir (tmp_dir, mode);

  if (tmp_dir != dir)
    free (tmp_dir);

  return ret_val;
}
