/* Work around the condition whereby calloc (n, s) fails when n*s is 0.
   This wrapper function is required at least on Tru64 UNIX 5.1.
   Copyright (C) 2004 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#undef calloc

#include <stdlib.h>

/* Allocate and zero-fill an NxS-byte block of memory from the heap.
   If N or S is zero, allocate and zero-fill a 1-byte block.  */

void *
rpl_calloc (size_t n, size_t s)
{
  if (n == 0)
    n = 1;
  if (s == 0)
    s = 1;
  return calloc (n, s);
}
