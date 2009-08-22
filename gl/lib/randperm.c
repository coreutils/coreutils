/* Generate random permutations.

   Copyright (C) 2006, 2007, 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert.  */

#include <config.h>

#include "randperm.h"

#include <limits.h>

#include "xalloc.h"

/* Return the ceiling of the log base 2 of N.  If N is zero, return
   an unspecified value.  */

static size_t
ceil_lg (size_t n)
{
  size_t b = 0;
  for (n--; n != 0; n /= 2)
    b++;
  return b;
}

/* Return an upper bound on the number of random bytes needed to
   generate the first H elements of a random permutation of N
   elements.  H must not exceed N.  */

size_t
randperm_bound (size_t h, size_t n)
{
  /* Upper bound on number of bits needed to generate the first number
     of the permutation.  */
  size_t lg_n = ceil_lg (n);

  /* Upper bound on number of bits needed to generated the first H elements.  */
  size_t ar = lg_n * h;

  /* Convert the bit count to a byte count.  */
  size_t bound = (ar + CHAR_BIT - 1) / CHAR_BIT;

  return bound;
}

/* From R, allocate and return a malloc'd array of the first H elements
   of a random permutation of N elements.  H must not exceed N.
   Return NULL if H is zero.  */

size_t *
randperm_new (struct randint_source *r, size_t h, size_t n)
{
  size_t *v;

  switch (h)
    {
    case 0:
      v = NULL;
      break;

    case 1:
      v = xmalloc (sizeof *v);
      v[0] = randint_choose (r, n);
      break;

    default:
      {
        size_t i;

        v = xnmalloc (n, sizeof *v);
        for (i = 0; i < n; i++)
          v[i] = i;

        for (i = 0; i < h; i++)
          {
            size_t j = i + randint_choose (r, n - i);
            size_t t = v[i];
            v[i] = v[j];
            v[j] = t;
          }

        v = xnrealloc (v, h, sizeof *v);
      }
      break;
    }

  return v;
}
