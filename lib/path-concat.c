/* path-concat.c -- concatenate two arbitrary pathnames
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef HAVE_MEMPCPY
# define mempcpy(D, S, N) ((void *) ((char *) memcpy (D, S, N) + (N)))
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include <sys/types.h>

char *malloc ();

/* Concatenate two pathname components, DIR and BASE, in newly-allocated
   storage and return the result.  Return 0 if out of memory.  Add a slash
   between DIR and BASE in the result if neither would contribute one.
   If each would contribute at least one, elide one from the end of DIR.
   Otherwise, simply concatenate DIR and BASE.  In any case, if
   BASE_IN_RESULT is non-NULL, set *BASE_IN_RESULT to point to the copy of
   BASE in the returned concatenation.  */

char *
path_concat (dir, base, base_in_result)
     const char *dir;
     const char *base;
     char **base_in_result;
{
  char *p;
  char *p_concat;
  size_t base_len = strlen (base);
  size_t dir_len = strlen (dir);

  p_concat = malloc (dir_len + base_len + 2);
  if (!p_concat)
    return 0;

  p = mempcpy (p_concat, dir, dir_len);

  if (*(p - 1) == '/' && *base == '/')
    --p;
  else if (*(p - 1) != '/' && *base != '/')
    *p++ = '/';

  if (base_in_result)
    *base_in_result = p;

  memcpy (p, base, base_len + 1);

  return p_concat;
}
