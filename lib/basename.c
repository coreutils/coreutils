/* basename.c -- return the last element in a path
   Copyright (C) 1990, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <assert.h>

#ifndef FILESYSTEM_PREFIX_LEN
# define FILESYSTEM_PREFIX_LEN(Filename) 0
#endif

#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

#ifndef ISSLASH
# define ISSLASH(C) ((C) == '/')
#endif

char *base_name PARAMS ((char const *name));

/* In general, we can't use the builtin `basename' function if available,
   since it has different meanings in different environments.
   In some environments the builtin `basename' modifies its argument.
   If NAME is all slashes, be sure to return `/'.  */

char *
base_name (char const *name)
{
  char const *base = name += FILESYSTEM_PREFIX_LEN (name);
  int all_slashes = 1;
  char const *p;

  for (p = name; *p; p++)
    {
      if (ISSLASH (*p))
	base = p + 1;
      else
	all_slashes = 0;
    }

  /* If NAME is all slashes, arrange to return `/'.  */
  if (*base == '\0' && ISSLASH (*name) && all_slashes)
    --base;

  /* Make sure the last byte is not a slash.  */
  assert (all_slashes || !ISSLASH (*(p - 1)));

  return (char *) base;
}
