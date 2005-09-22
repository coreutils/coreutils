/* Concatenate two arbitrary file names.

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005 Free
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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "filenamecat.h"

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

/* Concatenate two file name components, DIR and ABASE, in
   newly-allocated storage and return the result.
   The resulting file name F is such that the commands "ls F" and "(cd
   DIR; ls BASE)" refer to the same file, where BASE is ABASE with any
   file system prefixes and leading separators removed.
   Arrange for a directory separator if necessary between DIR and BASE
   in the result, removing any redundant separators.
   In any case, if BASE_IN_RESULT is non-NULL, set
   *BASE_IN_RESULT to point to the copy of ABASE in the returned
   concatenation.  However, if ABASE begins with more than one slash,
   set *BASE_IN_RESULT to point to the sole corresponding slash that
   is copied into the result buffer.

   Report an error if memory is exhausted.  */

char *
file_name_concat (char const *dir, char const *abase, char **base_in_result)
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
    *base_in_result = p - IS_ABSOLUTE_FILE_NAME (abase);

  p = mempcpy (p, base, baselen);
  *p = '\0';

  return p_concat;
}

#ifdef TEST_FILE_NAME_CONCAT
# include <stdlib.h>
# include <stdio.h>
int
main ()
{
  static char const *const tests[][3] =
    {
      {"a", "b",   "a/b"},
      {"a/", "b",  "a/b"},
      {"a/", "/b", "a/b"},
      {"a", "/b",  "a/b"},

      {"/", "b",  "/b"},
      {"/", "/b", "/b"},
      {"/", "/",  "/"},
      {"a", "/",  "a/"},   /* this might deserve a diagnostic */
      {"/a", "/", "/a/"},  /* this might deserve a diagnostic */
      {"a", "//b",  "a/b"},
    };
  size_t i;
  bool fail = false;
  for (i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
      char *base_in_result;
      char const *const *t = tests[i];
      char *res = file_name_concat (t[0], t[1], &base_in_result);
      if (strcmp (res, t[2]) != 0)
	{
	  printf ("got %s, expected %s\n", res, t[2]);
	  fail = true;
	}
    }
  exit (fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
#endif
