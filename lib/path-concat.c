/* path-concat.c -- concatenate two arbitrary pathnames

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004 Free
   Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "path-concat.h"

#include <string.h>

#include "dirname.h"
#include "xalloc.h"

#if ! HAVE_MEMPCPY && ! defined mempcpy
# define mempcpy(D, S, N) ((void *) ((char *) memcpy (D, S, N) + (N)))
#endif

/* Return the longest suffix of F that is a relative file name.
   If it has no such suffix, return the empty string.  */

static char const *
longest_relative_suffix (char const *f)
{
  for (f += FILE_SYSTEM_PREFIX_LEN (f); ISSLASH (*f); f++)
    continue;
  return f;
}

/* Concatenate two pathname components, DIR and ABASE, in
   newly-allocated storage and return the result.
   The resulting file name F is such that the commands "ls F" and "(cd
   DIR; ls BASE)" refer to the same file, where BASE is ABASE with any
   file system prefixes and leading separators removed.
   Arrange for a directory separator if necessary between DIR and BASE
   in the result, removing any redundant separators.
   In any case, if BASE_IN_RESULT is non-NULL, set
   *BASE_IN_RESULT to point to the copy of BASE in the returned
   concatenation.

   Report an error if memory is exhausted.  */

char *
path_concat (char const *dir, char const *abase, char **base_in_result)
{
  char const *dirbase = base_name (dir);
  size_t dirbaselen = base_len (dirbase);
  size_t dirlen = dirbase - dir + dirbaselen;
  size_t needs_separator = (dirbaselen && ! ISSLASH (dirbase[dirbaselen - 1]));

  char const *base = longest_relative_suffix (abase);
  size_t baselen = strlen (base);

  char *p_concat = xmalloc (dirlen + needs_separator + baselen + 1);
  char *p;

  p = mempcpy (p_concat, dir, dirlen);
  *p = DIRECTORY_SEPARATOR;
  p += needs_separator;

  if (base_in_result)
    *base_in_result = p;

  p = mempcpy (p, base, baselen);
  *p = '\0';

  return p_concat;
}
