/* @IGNORE@ -*- c -*- */
/* Work around the bug in some systems whereby rename fails when the
   source path has a trailing slash.  The rename from SunOS 4.1.1_U1
   has this bug.
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

/* written by Volker Borchert */

#include <config.h>
#include <stdio.h>

#ifndef HAVE_DECL_FREE
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_FREE
void free ();
#endif

/* Rename the file SRC_PATH to the file DST_PATH, removing any trailing
   slashes from SRC_PATH. Needed for SunOS 4.1.1_U1.  */

int
rpl_rename (const char *src_path, const char *dst_path)
{
  char *src_temp;
  int i;
  int t;

  i = strlen (src_path) - 1;
  if (src_path[i] == '/')
    {
      src_temp = xstrdup (src_path);
      for ( ; i > 0 && src_path[i] == '/'; i--)
	src_temp[i] = '\0';
    }
  else
    src_temp = (char *) src_path;

  t = rename (src_temp, dst_path);

  if (src_temp != src_path)
    free (src_temp);

  return t;
}
