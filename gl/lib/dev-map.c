/* Map a dev_t device number to a small integer.

   Copyright 2009-2010 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>
#include "dev-map.h"

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

struct dev_map_ent
{
  dev_t dev;
  uint32_t mapped_dev;
};

enum { INITIAL_DEV_MAP_TABLE_SIZE = 31 };

static size_t
dev_hash (void const *x, size_t table_size)
{
  struct dev_map_ent const *p = x;
  return (uintmax_t) p->dev % table_size;
}

/* Compare two dev_map_ent structs on "dev".
   Return true if they are the same.  */
static bool
dev_compare (void const *x, void const *y)
{
  struct dev_map_ent const *a = x;
  struct dev_map_ent const *b = y;
  return a->dev == b->dev ? true : false;
}

/* Initialize state.  */
int
dev_map_init (struct dev_map *dev_map)
{
  assert (dev_map != NULL);
  dev_map->n_device = 0;
  dev_map->dev_map = hash_initialize (INITIAL_DEV_MAP_TABLE_SIZE, NULL,
                                            dev_hash, dev_compare, free);
  return dev_map->dev_map ? 0 : -1;
}

void
dev_map_free (struct dev_map *dev_map)
{
  hash_free (dev_map->dev_map);
}

/* Insert device number, DEV, into the map, DEV_MAP if it's not there already,
   and return the nonnegative, mapped and usually smaller, number.
   If DEV is already in DEV_MAP, return the existing mapped value.
   Upon memory allocation failure, return -1.  */
int
dev_map_insert (struct dev_map *dev_map, dev_t dev)
{
  /* Attempt to insert <DEV,n_device> in the map.
     Possible outcomes:
     - DEV was already there, with a different necessarily smaller value
     - DEV was not there, (thus was just inserted)
     - insert failed: out of memory
     Return -1 on out of memory.
  */

  struct dev_map_ent *ent_from_table;
  struct dev_map_ent *ent = malloc (sizeof *ent);
  if (!ent)
    return -1;

  ent->dev = dev;
  ent->mapped_dev = dev_map->n_device;

  ent_from_table = hash_insert (dev_map->dev_map, ent);
  if (ent_from_table == NULL)
    {
      /* Insertion failed due to lack of memory.  */
      return -1;
    }

  if (ent_from_table == ent)
    {
      /* Insertion succeeded: new device.  */
      return dev_map->n_device++;
    }

  /* That DEV value is already in the table, so ENT was not inserted.
     Free it and return the already-associated value.  */
  free (ent);
  return ent_from_table->mapped_dev;
}
