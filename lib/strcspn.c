/* Copyright (C) 1991 Free Software Foundation, Inc.

NOTE: The canonical source of this file is maintained with the GNU C Library.
Bugs can be reported to bug-glibc@prep.ai.mit.edu.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <ansidecl.h>
#include <string.h>


/* Return the length of the maximum inital segment of S
   which contains no characters from REJECT.  */
size_t
DEFUN(strcspn, (s, reject),
      register CONST char *s AND register CONST char *reject)
{
  register size_t count = 0;

  while (*s != '\0')
    if (strchr(reject, *s++) == NULL)
      ++count;
    else
      return count;

  return count;
}
