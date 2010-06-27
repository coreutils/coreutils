/* Test the dev-map module.
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

/* Written by Jim Meyering.  */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* FIXME: once/if in gnulib, use #include "macros.h" in place of this */
#define ASSERT(expr) \
  do                                                                         \
    {                                                                        \
      if (!(expr))                                                           \
        {                                                                    \
          fprintf (stderr, "%s:%d: assertion failed\n", __FILE__, __LINE__); \
          fflush (stderr);                                                   \
          abort ();                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

#include "dev-map.h"

/* Risky: this is also defined in di-set.c; they should match.  */
#define N_DEV_BITS_4 5

int
main ()
{
  /* set_program_name (argv[0]); placate overzealous "syntax-check" test.  */
  struct dev_map dev_map;
  ASSERT (dev_map_init (&dev_map) == 0);

  ASSERT (dev_map_insert (&dev_map, 42) == 0);
  ASSERT (dev_map_insert (&dev_map, 42) == 0);
  ASSERT (dev_map_insert (&dev_map, 398) == 1);
  ASSERT (dev_map_insert (&dev_map, 398) == 1);

  int32_t i;
  for (i = 0; i < (1 << N_DEV_BITS_4); i++)
    {
      ASSERT (dev_map_insert (&dev_map, 10000+i) == 2+i);
    }

  dev_map_free (&dev_map);

  return 0;
}
