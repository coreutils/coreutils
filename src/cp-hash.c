/* cp-hash.c  -- file copying (hash search routines)
   Copyright (C) 89, 90, 91, 1995-2001 Free Software Foundation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Torbjorn Granlund, Sweden (tege@sics.se). */

#include <config.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "error.h"
#include "cp-hash.h"
#include "quote.h"

struct entry
{
  ino_t ino;
  dev_t dev;
  /* Destination path name (of non-directory or pre-existing directory)
     corresponding to the dev/ino of a copied file, or the destination path
     name corresponding to a dev/ino pair for a newly-created directory. */
  char *node;

  struct entry *coll_link;	/* 0 = entry not occupied.  */
};

struct htab
{
  unsigned modulus;		/* Size of the `hash' pointer vector.  */
  struct entry *entry_tab;	/* Pointer to dynamically growing vector.  */
  unsigned entry_tab_size;	/* Size of current `entry_tab' allocation.  */
  unsigned first_free_entry;	/* Index in `entry_tab'.  */
  struct entry *hash[1];	/* Vector of pointers in `entry_tab'.  */
};

static struct htab *htab;

static char *cph_hash_insert PARAMS ((ino_t ino, dev_t dev, const char *node));

/* Add PATH to the list of files that we have created.
   Return 0 if successful, 1 if not. */

int
remember_created (const char *path)
{
  struct stat sb;

  if (stat (path, &sb) < 0)
    {
      error (0, errno, "%s", quote (path));
      return 1;
    }

  cph_hash_insert (sb.st_ino, sb.st_dev, path);
  return 0;
}

/* Add path NODE, copied from inode number INO and device number DEV,
   to the list of files we have copied.
   Return NULL if inserted, otherwise non-NULL. */

char *
remember_copied (const char *node, ino_t ino, dev_t dev)
{
  return cph_hash_insert (ino, dev, node);
}

/* Allocate space for the hash structures, and set the global
   variable `htab' to point to it.  The initial hash module is specified in
   MODULUS, and the number of entries are specified in ENTRY_TAB_SIZE.  (The
   hash structure will be rebuilt when ENTRY_TAB_SIZE entries have been
   inserted, and MODULUS and ENTRY_TAB_SIZE in the global `htab' will be
   doubled.)  */

void
hash_init (unsigned int modulus, unsigned int entry_tab_size)
{
  struct htab *htab_r;

  htab_r = (struct htab *)
    xmalloc (sizeof (struct htab) + sizeof (struct entry *) * modulus);

  htab_r->entry_tab = (struct entry *)
    xmalloc (sizeof (struct entry) * entry_tab_size);

  htab_r->modulus = modulus;
  htab_r->entry_tab_size = entry_tab_size;
  htab = htab_r;

  forget_all ();
}

/* Reset the hash structure in the global variable `htab' to
   contain no entries.  */

void
forget_all (void)
{
  int i;
  struct entry **p;

  htab->first_free_entry = 0;

  p = htab->hash;
  for (i = htab->modulus; i > 0; i--)
    *p++ = NULL;
}

/* Insert path NODE, copied from inode number INO and device number DEV,
   into the hash structure HTAB, if not already present.
   Return NULL if inserted, otherwise non-NULL. */

static char *
hash_insert2 (struct htab *ht, ino_t ino, dev_t dev, const char *node)
{
  struct entry **hp, *ep2, *ep;

  /* The cast to uintmax_t prevents negative remainders if ino is negative.  */
  hp = &ht->hash[(uintmax_t) ino % ht->modulus];

  ep2 = *hp;

  /* Collision?  */

  if (ep2 != NULL)
    {
      ep = ep2;

      /* Search for an entry with the same data.  */

      do
	{
	  if (ep->ino == ino && ep->dev == dev)
	    return ep->node;	/* Found an entry with the same data.  */
	  ep = ep->coll_link;
	}
      while (ep != NULL);

      /* Did not find it.  */

    }

  ep = *hp = &ht->entry_tab[ht->first_free_entry++];
  ep->ino = ino;
  ep->dev = dev;
  ep->node = (char *) node;
  ep->coll_link = ep2;		/* ep2 is NULL if not collision.  */

  return NULL;
}

/* Insert path NODE, copied from inode number INO and device number DEV,
   into the hash structure in the global variable `htab', if an entry with
   the same inode and device was not found already.
   Return NULL if inserted, otherwise non-NULL. */

static char *
cph_hash_insert (ino_t ino, dev_t dev, const char *node)
{
  struct htab *htab_r = htab;

  if (htab_r->first_free_entry >= htab_r->entry_tab_size)
    {
      int i;
      struct entry *ep;
      unsigned modulus;
      unsigned entry_tab_size;

      /* Increase the number of hash entries, and re-hash the data.
	 The method of shrinking and increasing is made to compactify
	 the heap.  If twice as much data would be allocated
	 straightforwardly, we would never re-use a byte of memory.  */

      /* Let htab shrink.  Keep only the header, not the pointer vector.  */

      htab_r = (struct htab *)
	xrealloc ((char *) htab_r, sizeof (struct htab));

      modulus = 2 * htab_r->modulus;
      entry_tab_size = 2 * htab_r->entry_tab_size;

      /* Increase the number of possible entries.  */

      htab_r->entry_tab = (struct entry *)
	xrealloc ((char *) htab_r->entry_tab,
		  sizeof (struct entry) * entry_tab_size);

      /* Increase the size of htab again.  */

      htab_r = (struct htab *)
	xrealloc ((char *) htab_r,
		  sizeof (struct htab) + sizeof (struct entry *) * modulus);

      htab_r->modulus = modulus;
      htab_r->entry_tab_size = entry_tab_size;
      htab = htab_r;

      i = htab_r->first_free_entry;

      /* Make the increased hash table empty.  The entries are still
	 available in htab->entry_tab.  */

      forget_all ();

      /* Go through the entries and install them in the pointer vector
	 htab->hash.  The items are actually inserted in htab->entry_tab at
	 the position where they already are.  The htab->coll_link need
	 however be updated.  Could be made a little more efficient.  */

      for (ep = htab_r->entry_tab; i > 0; i--)
	{
	  hash_insert2 (htab_r, ep->ino, ep->dev, ep->node);
	  ep++;
	}
    }

  return hash_insert2 (htab_r, ino, dev, node);
}
