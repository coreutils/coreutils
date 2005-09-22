/* stripslash.c -- remove redundant trailing slashes from a file name

   Copyright (C) 1990, 2001, 2003, 2004, 2005 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "dirname.h"

/* Remove trailing slashes from FILE.
   Return true if a trailing slash was removed.
   This is useful when using file name completion from a shell that
   adds a "/" after directory names (such as tcsh and bash), because
   the Unix rename and rmdir system calls return an "Invalid argument" error
   when given a file that ends in "/" (except for the root directory).  */

bool
strip_trailing_slashes (char *file)
{
  char *base = base_name (file);
  char *base_lim = base + base_len (base);
  bool had_slash = (*base_lim != '\0');
  *base_lim = '\0';
  return had_slash;
}
