/* Barebones heap implementation supporting only insert and pop.

   Copyright (C) 2010-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Full implementation: GDSL (http://gna.org/projects/gdsl/) by Nicolas
   Darnis <ndarnis@free.fr>. */

#include <config.h>

#include "heap.h"
#include "stdlib--.h"
#include "xalloc.h"

static int heap_default_compare (void const *, void const *);
static void heapify_down (void **, idx_t, idx_t,
                          int (*) (void const *, void const *));
static void heapify_up (void **, idx_t,
                        int (*) (void const *, void const *));

struct heap
{
  void **array;     /* array[0] is not used */
  idx_t capacity;   /* Array size */
  idx_t count;      /* Used as index to last element. Also is num of items. */
  int (*compare) (void const *, void const *);
};

/* Allocate memory for the heap. */

struct heap *
heap_alloc (int (*compare) (void const *, void const *), idx_t n_reserve)
{
  struct heap *heap = xmalloc (sizeof *heap);

  if (n_reserve == 0)
    n_reserve = 1;

  heap->array = xnmalloc (n_reserve, sizeof *(heap->array));

  heap->array[0] = nullptr;
  heap->capacity = n_reserve;
  heap->count = 0;
  heap->compare = compare ? compare : heap_default_compare;

  return heap;
}


static int
heap_default_compare (void const *a, void const *b)
{
  return 0;
}


void
heap_free (struct heap *heap)
{
  free (heap->array);
  free (heap);
}

/* Insert element into heap. */

int
heap_insert (struct heap *heap, void *item)
{
  if (heap->capacity - 1 <= heap->count)
    heap->array = xpalloc (heap->array, &heap->capacity, 1, -1,
                           sizeof heap->array[0]);

  heap->array[++heap->count] = item;
  heapify_up (heap->array, heap->count, heap->compare);

  return 0;
}

/* Pop top element off heap. */

void *
heap_remove_top (struct heap *heap)
{
  void *top;

  if (heap->count == 0)
    return nullptr;

  top = heap->array[1];
  heap->array[1] = heap->array[heap->count--];
  heapify_down (heap->array, heap->count, 1, heap->compare);

  return top;
}

/* Move element down into appropriate position in heap. */

static void
heapify_down (void **array, idx_t count, idx_t initial,
              int (*compare) (void const *, void const *))
{
  void *element = array[initial];

  idx_t parent = initial;
  while (parent <= count >> 1)
    {
      idx_t child = 2 * parent;

      if (child < count && compare (array[child], array[child + 1]) < 0)
        child++;

      if (compare (array[child], element) <= 0)
        break;

      array[parent] = array[child];
      parent = child;
    }

  array[parent] = element;
}

/* Move element up into appropriate position in heap. */

static void
heapify_up (void **array, idx_t count,
            int (*compare) (void const *, void const *))
{
  idx_t k = count;
  void *new_element = array[k];

  while (k != 1 && compare (array[k >> 1], new_element) <= 0)
    {
      array[k] = array[k >> 1];
      k >>= 1;
    }

  array[k] = new_element;
}
