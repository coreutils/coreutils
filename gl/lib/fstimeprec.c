/* Determine a file system's time stamp precision.

   Copyright 2010 Free Software Foundation, Inc.

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
#include "fstimeprec.h"

#include "hash.h"
#include "stat-time.h"
#include <limits.h>
#include <stdlib.h>

/* A hash table that maps device numbers to precisions.  */
struct fstimeprec
{
  /* Map device numbers to precisions.  */
  struct hash_table *map;

  /* Cache of the most recently allocated and otherwise-unused storage
     for probing this table.  */
  struct ent *probe;
};

/* A pair that maps a device number to a precision.  */
struct ent
{
  dev_t dev;
  int prec;
};

/* Hash an entry.  */
static size_t
ent_hash (void const *x, size_t table_size)
{
  struct ent const *p = x;
  dev_t dev = p->dev;

  /* FIXME: This code is duplicated from di-set.c.  */
  /* When DEV is wider than size_t, exclusive-OR the words of DEV into H.
     This avoids loss of info, without applying % to the wider type,
     which could be quite slow on some systems.  */
  size_t h = dev;
  unsigned int i;
  unsigned int n_words = sizeof dev / sizeof h + (sizeof dev % sizeof h != 0);
  for (i = 1; i < n_words; i++)
    h ^= dev >> CHAR_BIT * sizeof h * i;

  return h % table_size;
}

/* Return true if two entries are the same.  */
static bool
ent_compare (void const *x, void const *y)
{
  struct ent const *a = x;
  struct ent const *b = y;
  return a->dev == b->dev;
}

/* Using the SET table, map a device to an entry that represents
   the file system precision.  Return NULL on error.  */
static struct ent *
map_device (struct fstimeprec *tab, dev_t dev)
{
  /* Find space for the probe, reusing the cache if available.  */
  struct ent *ent;
  struct ent *probe = tab->probe;
  if (probe)
    {
      /* If repeating a recent query, return the cached result.   */
      if (probe->dev == dev)
        return probe;
    }
  else
    {
      tab->probe = probe = malloc (sizeof *probe);
      if (! probe)
        return NULL;
      probe->prec = 0;
    }

  /* Probe for the device.  */
  probe->dev = dev;
  ent = hash_insert (tab->map, probe);
  if (ent == probe)
    tab->probe = NULL;
  return ent;
}

/* Return a newly allocated table of file system time stamp
   resolutions, or NULL if out of memory.  */
struct fstimeprec *
fstimeprec_alloc (void)
{
  struct fstimeprec *tab = malloc (sizeof *tab);
  if (tab)
    {
      enum { INITIAL_DEV_MAP_SIZE = 11 };
      tab->map = hash_initialize (INITIAL_DEV_MAP_SIZE, NULL,
                                  ent_hash, ent_compare, free);
      if (! tab->map)
        {
          free (tab);
          return NULL;
        }
      tab->probe = NULL;
    }
  return tab;
}

/* Free TAB.  */
void
fstimeprec_free (struct fstimeprec *tab)
{
  hash_free (tab->map);
  free (tab->probe);
  free (tab);
}

/* Using the cached information in TAB, return the estimated precision
   of time stamps for the file system containing the file whose status
   info is in ST.  The returned value counts the number of decimal
   fraction digits.  Return 0 on failure.

   If TAB is null, the guess is based purely on ST.  */
int
fstimeprec (struct fstimeprec *tab, struct stat const *st)
{
  /* Map the device number to an entry.  */
  struct ent *ent = (tab ? map_device (tab, st->st_dev) : NULL);

  /* Guess the precision based on what has been seen so far.  */
  int prec = (ent ? ent->prec : 0);

  /* If the current file's timestamp resolution is higher than the
     guess, increase the guess.  */
  if (prec < 9)
    {
      struct timespec ats = get_stat_atime (st);
      struct timespec mts = get_stat_mtime (st);
      struct timespec cts = get_stat_ctime (st);
      struct timespec bts = get_stat_birthtime (st);
      unsigned int ans = ats.tv_nsec;
      unsigned int mns = mts.tv_nsec;
      unsigned int cns = cts.tv_nsec;
      unsigned int bns = bts.tv_nsec < 0 ? 0 : bts.tv_nsec;
      unsigned int power = 10;
      int p;
      for (p = prec + 1; p < 9; p++)
        power *= 10;

      while (ans % power | mns % power | cns % power | bns % power)
        {
          power /= 10;
          prec++;
        }

      if (ent)
        ent->prec = prec;
    }

  return prec;
}
