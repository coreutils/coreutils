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
#include <stdint.h>

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

#include "di-set.h"

/* FIXME: ugly duplication of code from di-set.c */
#define N_DEV_BITS_4 5
#define N_INO_BITS_4 (32 - N_DEV_BITS_4 - 2 - 1)

#define N_DEV_BITS_8 8
#define N_INO_BITS_8 (64 - N_DEV_BITS_8 - 2 - 1)

int
main (void)
{
  /* set_program_name (argv[0]); placate overzealous "syntax-check" test.  */
  size_t initial_size = 61;
  /* "real" code might prefer to avoid the allocation here, simply
     declaring "struct di_set_state dis;", do a global substitution,
     s/\<dis\>/\&dis/, and remove the final free.  */
  struct di_set_state *dis = malloc (sizeof *dis);
  ASSERT (dis);
  ASSERT (di_set_init (dis, initial_size) == 0);

  struct di_ent *di_ent;
  ASSERT (di_ent_create (dis, 1, 1, &di_ent) == 0);
  ASSERT (di_ent_create (dis, 1 << N_DEV_BITS_4, 1, &di_ent) == 0);
  ASSERT (di_ent_create (dis, 1, 1 << N_INO_BITS_4, &di_ent) == 0);
  ASSERT (di_ent_create (dis, 1,
                         (uint64_t) 1 << N_INO_BITS_8, &di_ent) == 0);
  free (di_ent);

  ASSERT (di_set_insert (dis, 2, 5) == 1); /* first insertion succeeds */
  ASSERT (di_set_insert (dis, 2, 5) == 0); /* duplicate fails */
  ASSERT (di_set_insert (dis, 3, 5) == 1); /* diff dev, duplicate inode is ok */
  ASSERT (di_set_insert (dis, 2, 8) == 1); /* same dev, different inode is ok */

  /* very large inode number */
  ASSERT (di_set_insert (dis, 5, (uint64_t) 1 << 63) == 1);
  ASSERT (di_set_insert (dis, 5, (uint64_t) 1 << 63) == 0); /* dup */

  unsigned int i;
  for (i = 0; i < 3000; i++)
    ASSERT (di_set_insert (dis, 9, i) == 1);
  for (i = 0; i < 3000; i++)
    ASSERT (di_set_insert (dis, 9, i) == 0); /* duplicate fails */

  di_set_free (dis);
  free (dis);

  return 0;
}
