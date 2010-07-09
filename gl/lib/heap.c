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
   Darnis <ndarnis@free.fr>. */

#include <config.h>

#include "heap.h"
#include "stdlib--.h"
#include "xalloc.h"

static int heap_default_compare (const void *, const void *);
static size_t heapify_down (void **, size_t, size_t,
                            int (*)(const void *, const void *));
static void heapify_up (void **, size_t,
                        int (*)(const void *, const void *));


/* Allocate memory for the heap. */

struct heap *
heap_alloc (int (*compare)(const void *, const void *), size_t n_reserve)
{
  struct heap *heap;
  void *xmalloc_ret = xmalloc (sizeof *heap);
  heap = (struct heap *) xmalloc_ret;
  if (!heap)
    return NULL;

  if (n_reserve <= 0)
    n_reserve = 1;

  xmalloc_ret = xmalloc (n_reserve * sizeof *(heap->array));
  heap->array = (void **) xmalloc_ret;
  if (!heap->array)
    {
      free (heap);
      return NULL;
    }

  heap->array[0] = NULL;
  heap->capacity = n_reserve;
  heap->count = 0;
  heap->compare = compare ? compare : heap_default_compare;

  return heap;
}


static int
heap_default_compare (const void *a, const void *b)
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
    {
      size_t new_size = (2 + heap->count) * sizeof *(heap->array);
      void *realloc_ret = xrealloc (heap->array, new_size);
      heap->array = (void **) realloc_ret;
      heap->capacity = (2 + heap->count);

      if (!heap->array)
        return -1;
    }

  heap->array[++heap->count] = item;
  heapify_up (heap->array, heap->count, heap->compare);

  return 0;
}

/* Pop top element off heap. */

void *
heap_remove_top (struct heap *heap)
{
  if (heap->count == 0)
    return NULL;

  void *top = heap->array[1];
  heap->array[1] = heap->array[heap->count--];
  heapify_down (heap->array, heap->count, 1, heap->compare);

  return top;
}

/* Move element down into appropriate position in heap. */

static size_t
heapify_down (void **array, size_t count, size_t initial,
              int (*compare)(const void *, const void *))
{
  void *element = array[initial];

  size_t parent = initial;
  while (parent <= count / 2)
    {
      size_t child = 2 * parent;

      if (child < count && compare (array[child], array[child+1]) < 0)
        child++;

      if (compare (array[child], element) <= 0)
        break;

      array[parent] = array[child];
      parent = child;
    }

  array[parent] = element;
  return parent;
}

/* Move element up into appropriate position in heap. */

static void
heapify_up (void **array, size_t count,
            int (*compare)(const void *, const void *))
{
  size_t k = count;
  void *new_element = array[k];

  while (k != 1 && compare (array[k/2], new_element) <= 0)
    {
      array[k] = array[k/2];
      k /= 2;
    }

  array[k] = new_element;
}
