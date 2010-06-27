/* Set operations for device-inode pairs stored in a space-efficient manner.

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
#include "di-set.h"

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "verify.h"

/* Set operations for device-inode pairs stored in a space-efficient manner.
   A naive mapping uses 16 bytes to save a single st_dev, st_ino pair.
   However, in many applications, the vast majority of actual device,inode
   number pairs can be efficiently compressed to fit in 8 or even 4 bytes,
   by using a separate table to map a relatively small number of devices
   to small integers.  */

#define N_DEV_BITS_4 5
#define N_INO_BITS_4 (32 - N_DEV_BITS_4 - 2 - 1)

#define N_DEV_BITS_8 8
#define N_INO_BITS_8 (64 - N_DEV_BITS_8 - 2 - 1)

/* Note how the last bit is always set.
   This is required, in order to be able to distinguish
   an encoded di_ent value from a malloc-returned pointer,
   which must be 4-byte-aligned or better.  */
struct dev_ino_4
{
  uint32_t mode:2; /* must be first */
  uint32_t short_ino:N_INO_BITS_4;
  uint32_t mapped_dev:N_DEV_BITS_4;
  uint32_t always_set:1;
};
verify (N_DEV_BITS_4 <= 8 * sizeof (int));
verify (sizeof (struct dev_ino_4) == 4);

struct dev_ino_8
{
  uint32_t mode:2; /* must be first */
  uint64_t short_ino:N_INO_BITS_8;
  uint32_t mapped_dev:N_DEV_BITS_8;
  uint32_t always_set:1;
};
verify (sizeof (struct dev_ino_8) == 8);

struct dev_ino_full
{
  uint32_t mode:2; /* must be first */
  dev_t dev;
  ino_t ino;
};

enum di_mode
{
  DI_MODE_4 = 1,
  DI_MODE_8 = 2,
  DI_MODE_FULL = 3
};

/*
     di_mode     raw_inode      mapped dev    always_set
          \____________|_______________\_____/
  4-byte | 2|          25            |  5  |1|          mapped_dev
         `----------------------------------------------------|-----.
  8-byte | 2|        53                                  |    8   |1|
         `----------------------------------------------------------'
*/
struct di_ent
{
  union
  {
    struct dev_ino_4 di4;
    struct dev_ino_8 di8;
    struct dev_ino_full full;
    uint32_t u32;
    uint64_t u64;
    void *ptr;
  } u;
};

struct dev_map_ent
{
  dev_t dev;
  uint32_t mapped_dev;
};

static inline bool
is_encoded_ptr (struct di_ent const *v)
{
  return (size_t) v % 4;
}

static struct di_ent
decode_ptr (struct di_ent const *v)
{
  if (!is_encoded_ptr (v))
    return *v;

  struct di_ent di;
  di.u.ptr = (void *) v;
  return di;
}

static size_t
di_ent_hash (void const *x, size_t table_size)
{
  struct di_ent e = decode_ptr (x);
  return (e.u.di4.mode == DI_MODE_4
          ? e.u.u32
          : (e.u.di4.mode == DI_MODE_8
             ? e.u.u64
             : e.u.full.ino)) % table_size;
}

/* Compare two di_ent structs.
   Return true if they are the same.  */
static bool
di_ent_compare (void const *x, void const *y)
{
  struct di_ent a = decode_ptr (x);
  struct di_ent b = decode_ptr (y);
  if (a.u.di4.mode != b.u.di4.mode)
    return false;

  if (a.u.di4.mode == DI_MODE_4)
    return (a.u.di4.short_ino == b.u.di4.short_ino
            && a.u.di4.mapped_dev == b.u.di4.mapped_dev);

  if (a.u.di8.mode == DI_MODE_8)
    return (a.u.di8.short_ino == b.u.di8.short_ino
            && a.u.di8.mapped_dev == b.u.di8.mapped_dev);

  return (a.u.full.ino == b.u.full.ino
          && a.u.full.dev == b.u.full.dev);
}

static void
di_ent_free (void *v)
{
  if ( ! is_encoded_ptr (v))
    free (v);
}

int
di_set_init (struct di_set_state *dis, size_t initial_size)
{
  if (dev_map_init (&dis->dev_map) < 0)
    return -1;

  dis->di_set = hash_initialize (initial_size, NULL,
                                 di_ent_hash, di_ent_compare, di_ent_free);
  return dis->di_set ? 0 : -1;
}

void
di_set_free (struct di_set_state *dis)
{
  dev_map_free (&dis->dev_map);
  hash_free (dis->di_set);
}

/* Given a device-inode set, DIS, create an entry for the DEV,INO
   pair, and store it in *V.  If possible, encode DEV,INO into the pointer
   itself, but if not, allocate space for a full "struct di_ent" and set *V
   to that pointer.  Upon memory allocation failure, return -1.
   Otherwise return 0.  */
int
di_ent_create (struct di_set_state *dis,
               dev_t dev, ino_t ino,
               struct di_ent **v)
{
  static int prev_m = -1;
  static dev_t prev_dev = -1;
  struct di_ent di_ent;
  int mapped_dev;

  if (dev == prev_dev)
    mapped_dev = prev_m;
  else
    {
      mapped_dev = dev_map_insert (&dis->dev_map, dev);
      if (mapped_dev < 0)
        return -1;
      prev_dev = dev;
      prev_m = mapped_dev;
    }

  if (mapped_dev < (1 << N_DEV_BITS_4)
      && ino < (1 << N_INO_BITS_4))
    {
#if lint
      /* When this struct is smaller than a pointer, initialize
         the pointer so tools like valgrind don't complain about
         the uninitialized bytes.  */
      if (sizeof di_ent.u.di4 < sizeof di_ent.u.ptr)
        di_ent.u.ptr = NULL;
#endif
      di_ent.u.di4.mode = DI_MODE_4;
      di_ent.u.di4.short_ino = ino;
      di_ent.u.di4.mapped_dev = mapped_dev;
      di_ent.u.di4.always_set = 1;
      *v = di_ent.u.ptr;
    }
  else if (mapped_dev < (1 << N_DEV_BITS_8)
           && ino < ((uint64_t) 1 << N_INO_BITS_8))
    {
      di_ent.u.di8.mode = DI_MODE_8;
      di_ent.u.di8.short_ino = ino;
      di_ent.u.di8.mapped_dev = mapped_dev;
      di_ent.u.di8.always_set = 1;
      *v = di_ent.u.ptr;
    }
  else
    {
      /* Handle the case in which INO is too large or in which (far less
         likely) we encounter hard-linked files on 2^N_DEV_BITS_8
         different devices.  */
      struct di_ent *p = malloc (sizeof *p);
      if (!p)
        return -1;
      assert ((size_t) p % 4 == 0);
      p->u.full.mode = DI_MODE_FULL;
      p->u.full.ino = ino;
      p->u.full.dev = dev;
      *v = p;
    }

  return 0;
}

/* Attempt to insert the DEV,INO pair into the set, DIS.
   If it matches a pair already in DIS, don't modify DIS and return 0.
   Otherwise, if insertion is successful, return 1.
   Upon any failure return -1.  */
int
di_set_insert (struct di_set_state *dis, dev_t dev, ino_t ino)
{
  struct di_ent *v;
  if (di_ent_create (dis, dev, ino, &v) < 0)
    return -1;

  int err = hash_insert0 (dis->di_set, v, NULL);
  if (err == -1) /* Insertion failed due to lack of memory.  */
    return -1;

  if (err == 1) /* Insertion succeeded.  */
    return 1;

  /* That pair is already in the table, so ENT was not inserted.  Free it.  */
  if (! is_encoded_ptr (v))
    free (v);

  return 0;
}
