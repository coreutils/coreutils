/* Test the fstimeprec module.
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

/* written by Paul Eggert */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

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

#include "fstimeprec.h"

static int
fstimeprec_file (struct fstimeprec *tab, char const *file)
{
  struct stat st;
  if (stat (file, &st) != 0)
    return 0;
  return fstimeprec (tab, &st);
}

int
main (void)
{
  struct fstimeprec *tab = fstimeprec_alloc ();
  ASSERT (tab);

  int m1 = fstimeprec_file (tab, "/");
  int m2 = fstimeprec_file (tab, ".");
  int m3 = fstimeprec_file (tab, "..");
  ASSERT (0 <= m1 && m1 <= 9);
  ASSERT (0 <= m2 && m2 <= 9);
  ASSERT (0 <= m3 && m3 <= 9);

  int n1 = fstimeprec_file (tab, "/");
  int n2 = fstimeprec_file (tab, ".");
  int n3 = fstimeprec_file (tab, "..");
  ASSERT (0 <= n1 && n1 <= 9);
  ASSERT (0 <= n2 && n2 <= 9);
  ASSERT (0 <= n3 && n3 <= 9);

  ASSERT (m1 <= n1);
  ASSERT (m2 <= n2);
  ASSERT (m3 <= n3);

  fstimeprec_free (tab);

  return 0;
}
