/* Copyright (C) 1991, 1994, 1996-1997, 2002 Free Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C Library.
   Bugs can be reported to bug-glibc@gnu.org.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>

#if defined _LIBC || HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
#endif

#undef strcspn

/* Return the length of the maximum initial segment of S
   which contains no characters from REJECT.  */
size_t
strcspn (const char *s, const char *reject)
{
  size_t count = 0;

  while (*s != '\0')
    if (strchr (reject, *s++) == NULL)
      ++count;
    else
      return count;

  return count;
}
