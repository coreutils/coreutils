/* realloc() function that is glibc compatible.
   Copyright (C) 1997, 2003, 2004 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef realloc

#include <stdlib.h>

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  If N is zero, change it to 1.  If P is NULL,
   use malloc.  */

void *
rpl_realloc (void *p, size_t n)
{
  if (n == 0)
    {
      n = 1;

      /* In theory realloc might fail, so don't rely on it to free.  */
      free (p);
      p = NULL;
    }

  if (p == NULL)
    return malloc (n);
  return realloc (p, n);
}
