/* dirname.c -- return all but the last element in a file name

   Copyright (C) 1990, 1998, 2000, 2001, 2003, 2004, 2005 Free Software
   Foundation, Inc.

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

#include <string.h>
#include "xalloc.h"

/* Return the length of `dirname (FILE)', or zero if FILE is
   in the working directory.  Works properly even if
   there are trailing slashes (by effectively ignoring them).  */
size_t
dir_len (char const *file)
{
  size_t prefix_length = FILE_SYSTEM_PREFIX_LEN (file);
  size_t length;

  /* Strip the basename and any redundant slashes before it.  */
  for (length = base_name (file) - file;  prefix_length < length;  length--)
    if (! ISSLASH (file[length - 1]))
      return length;

  /* But don't strip the only slash from "/".  */
  return prefix_length + ISSLASH (file[prefix_length]);
}

/* Return the leading directories part of FILE,
   allocated with xmalloc.
   Works properly even if there are trailing slashes
   (by effectively ignoring them).  */

char *
dir_name (char const *file)
{
  size_t length = dir_len (file);
  bool append_dot = (length == FILE_SYSTEM_PREFIX_LEN (file));
  char *dir = xmalloc (length + append_dot + 1);
  memcpy (dir, file, length);
  if (append_dot)
    dir[length++] = '.';
  dir[length] = 0;
  return dir;
}

#ifdef TEST_DIRNAME
/*

Run the test like this (expect no output):
  gcc -DHAVE_CONFIG_H -DTEST_DIRNAME -I.. -O -Wall \
     basename.c dirname.c xmalloc.c error.c
  sed -n '/^BEGIN-DATA$/,/^END-DATA$/p' dirname.c|grep -v DATA|./a.out

If it's been built on a DOS or Windows platforms, run another test like
this (again, expect no output):
  sed -n '/^BEGIN-DOS-DATA$/,/^END-DOS-DATA$/p' dirname.c|grep -v DATA|./a.out

BEGIN-DATA
foo//// .
bar/foo//// bar
foo/ .
/ /
. .
a .
END-DATA

BEGIN-DOS-DATA
c:///// c:/
c:/ c:/
c:/. c:/
c:foo c:.
c:foo/bar c:foo
END-DOS-DATA

*/

# define MAX_BUFF_LEN 1024
# include <stdio.h>

char *program_name;

int
main (int argc, char *argv[])
{
  char buff[MAX_BUFF_LEN + 1];

  program_name = argv[0];

  buff[MAX_BUFF_LEN] = 0;
  while (fgets (buff, MAX_BUFF_LEN, stdin) && buff[0])
    {
      char file[MAX_BUFF_LEN];
      char expected_result[MAX_BUFF_LEN];
      char const *result;
      sscanf (buff, "%s %s", file, expected_result);
      result = dir_name (file);
      if (strcmp (result, expected_result))
	printf ("%s: got %s, expected %s\n", file, result, expected_result);
    }
  return 0;
}
#endif
