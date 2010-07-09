/* Barebones heap implementation supporting only insert and pop.

   Copyright (C) 2010 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Full implementation: GDSL (http://gna.org/projects/gdsl/) by Nicolas
   Darnis <ndarnis@free.fr>. Adapted by Gene Auyeung. */

#include <stddef.h>

struct heap
{
  void **array;     /* array[0] is not used */
  size_t capacity;  /* Array size */
  size_t count;     /* Used as index to last element. Also is num of items. */
  int (*compare)(const void *, const void *);
};

struct heap *heap_alloc (int (*)(const void *, const void *), size_t);
void heap_free (struct heap *);
int heap_insert (struct heap *heap, void *item);
void *heap_remove_top (struct heap *heap);
