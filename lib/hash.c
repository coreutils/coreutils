/* Global assumptions:
   - ANSI C
   - a certain amount of library support, at least <stdlib.h>
   - C ints are at least 32-bits long
 */

/* Things to do:
   - add a sample do_all function for listing the hash table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "near-prime.h"
#include "hash.h"

#ifdef USE_OBSTACK
/* This macro assumes that there is an HT with an initialized
   HT_OBSTACK in scope.  */
# define ZALLOC(n) obstack_alloc (&(ht->ht_obstack), (n))
#else
# define ZALLOC(n) malloc ((n))
#endif

#define BUCKET_HEAD(ht, idx) ((ht)->hash_table[(idx)])

static void hash_free_0 (HT *, int);

static void
hash_free_entry (HT *ht, HASH_ENT *e)
{
  e->key = NULL;
  e->next = ht->hash_free_entry_list;
  ht->hash_free_entry_list = e;
}

static HASH_ENT *
hash_allocate_entry (HT *ht)
{
  HASH_ENT *new;
  if (ht->hash_free_entry_list)
    {
      new = ht->hash_free_entry_list;
      ht->hash_free_entry_list = new->next;
    }
  else
    {
      new = (HASH_ENT *) ZALLOC (sizeof (HASH_ENT));
    }
  return new;
}

unsigned int
hash_get_n_slots_used (const HT *ht)
{
  return ht->hash_n_slots_used;
}

/* FIXME-comment */

int
hash_rehash (HT *ht, unsigned int new_table_size)
{
  HT *ht_new;
  unsigned int i;

  if (ht->hash_table_size <= 0 || new_table_size == 0)
    return 1;

  ht_new = hash_initialize (new_table_size, ht->hash_key_freer,
			    ht->hash_hash, ht->hash_key_comparator);

  if (ht_new == NULL)
    return 1;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *p = BUCKET_HEAD (ht, i);
      for ( /* empty */ ; p; p = p->next)
	{
	  int failed;
	  const void *already_in_table;
	  already_in_table = hash_insert_if_absent (ht_new, p->key, &failed);
	  assert (failed == 0 && already_in_table == 0);
	}
    }

  hash_free_0 (ht, 0);

#ifdef TESTING
  assert (hash_table_ok (ht_new));
#endif
  *ht = *ht_new;
  free (ht_new);

  /* FIXME: fill in ht_new->n_slots_used and other statistics fields. */

  return 0;
}

/* FIXME-comment */

unsigned int
hash_get_max_chain_length (HT *ht)
{
  unsigned int i;
  unsigned int max_chain_length = 0;

  if (!ht->hash_dirty_max_chain_length)
    return ht->hash_max_chain_length;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      unsigned int chain_length = 0;
      HASH_ENT *p = BUCKET_HEAD (ht, i);
      for ( /* empty */ ; p; p = p->next)
	++chain_length;
      if (chain_length > max_chain_length)
	max_chain_length = chain_length;
    }

  ht->hash_max_chain_length = max_chain_length;
  ht->hash_dirty_max_chain_length = 0;
  return ht->hash_max_chain_length;
}

unsigned int
hash_get_n_keys (const HT *ht)
{
  return ht->hash_n_keys;
}

unsigned int
hash_get_table_size (const HT *ht)
{
  return ht->hash_table_size;
}

/* TABLE_SIZE should be prime.  If WHEN_TO_REHASH is positive, when
   that percentage of table entries have been used, the table is
   deemed too small;  then a new, larger table (GROW_FACTOR times
   larger than the previous size) is allocated and all entries in
   the old table are rehashed into the new, larger one.  The old
   table is freed.  If WHEN_TO_REHASH is zero or negative, the
   table is never resized.

   The function returns non-zero
   - if TABLE_SIZE is zero or negative
   - if EQUALITY_TESTER or HASH is null
   - if it was unable to allocate sufficient storage for the hash table
   - if WHEN_TO_REHASH is zero or negative
   Otherwise it returns zero.

   FIXME: tell what happens to any existing hash table when this
   function is called (e.g. a second time).  */

HT *
hash_initialize (unsigned int table_size,
		 Hash_key_freer_type key_freer,
		 unsigned int (*hash) (const void *, unsigned int),
		 int (*key_comparator) (const void *, const void *))
{
  HT *ht;
  unsigned int i;

  if (table_size <= 0)
    return NULL;

  if (hash == NULL || key_comparator == NULL)
    return NULL;

  ht = (HT *) malloc (sizeof (HT));
  if (ht == NULL)
    return NULL;

  ht->hash_table = (HASH_ENT **) malloc (table_size * sizeof (HASH_ENT *));
  if (ht->hash_table == NULL)
    return NULL;

  for (i = 0; i < table_size; i++)
    {
      BUCKET_HEAD (ht, i) = NULL;
    }

  ht->hash_free_entry_list = NULL;
  ht->hash_table_size = table_size;
  ht->hash_hash = hash;
  ht->hash_key_comparator = key_comparator;
  ht->hash_key_freer = key_freer;
  ht->hash_n_slots_used = 0;
  ht->hash_max_chain_length = 0;
  ht->hash_n_keys = 0;
  ht->hash_dirty_max_chain_length = 0;
#ifdef USE_OBSTACK
  obstack_init (&(ht->ht_obstack));
#endif

  return ht;
}

/* This private function is used to help with insertion and deletion.
   If E does *not* compare equal to the key of any entry in the table,
   return NULL.
   When E matches an entry in the table, return a pointer to the matching
   entry.  When DELETE is non-zero and E matches an entry in the table,
   unlink the matching entry.  Set *CHAIN_LENGTH to the number of keys
   that have hashed to the bucket E hashed to.  */

static HASH_ENT *
hash_find_entry (HT *ht, const void *e, unsigned int *table_idx,
		 unsigned int *chain_length, int delete)
{
  unsigned int idx;
  int found;
  HASH_ENT *p, *prev;

  idx = ht->hash_hash (e, ht->hash_table_size);
  assert (idx < ht->hash_table_size);

  *table_idx = idx;
  *chain_length = 0;

  prev = ht->hash_table[idx];

  if (prev == NULL)
    return NULL;

  *chain_length = 1;
  if (ht->hash_key_comparator (e, prev->key) == 0)
    {
      if (delete)
	ht->hash_table[idx] = prev->next;
      return prev;
    }

  p = prev->next;
  found = 0;
  while (p)
    {
      ++(*chain_length);
      if (ht->hash_key_comparator (e, p->key) == 0)
	{
	  found = 1;
	  break;
	}
      prev = p;
      p = p->next;
    }

  if (!found)
    return NULL;

  assert (p != NULL);
  if (delete)
    prev->next = p->next;

  return p;
}

/* Return non-zero if E is already in the table, zero otherwise. */

int
hash_query_in_table (const HT *ht, const void *e)
{
  unsigned int idx;
  HASH_ENT *p;

  idx = ht->hash_hash (e, ht->hash_table_size);
  assert (idx < ht->hash_table_size);
  for (p = BUCKET_HEAD (ht, idx); p != NULL; p = p->next)
    if (ht->hash_key_comparator (e, p->key) == 0)
      return 1;
  return 0;
}

void *
hash_lookup (const HT *ht, const void *e)
{
  unsigned int idx;
  HASH_ENT *p;

  idx = ht->hash_hash (e, ht->hash_table_size);
  assert (idx < ht->hash_table_size);
  for (p = BUCKET_HEAD (ht, idx); p != NULL; p = p->next)
    if (ht->hash_key_comparator (e, p->key) == 0)
      return p->key;
  return NULL;
}

/* If E matches an entry already in the hash table, don't modify the
   table and return a pointer to the matched entry.  If E does not
   match any item in the table, insert E and return NULL.
   If the storage required for insertion cannot be allocated
   set *FAILED to non-zero and return NULL. */

void *
hash_insert_if_absent (HT *ht, const void *e, int *failed)
{
  const HASH_ENT *ent;
  HASH_ENT *new;
  unsigned int idx;
  unsigned int chain_length;

  assert (e != NULL);		/* Can't insert a NULL key. */

  *failed = 0;
  ent = hash_find_entry (ht, e, &idx, &chain_length, 0);
  if (ent != NULL)
    {
      /* E matches a key from an entry already in the table. */
      return ent->key;
    }

  new = hash_allocate_entry (ht);
  if (new == NULL)
    {
      *failed = 1;
      return NULL;
    }

  new->key = (void *) e;
  new->next = BUCKET_HEAD (ht, idx);
  BUCKET_HEAD (ht, idx) = new;

  if (chain_length == 0)
    ++(ht->hash_n_slots_used);

  /* The insertion has just increased chain_length by 1. */
  ++chain_length;

  if (chain_length > ht->hash_max_chain_length)
    ht->hash_max_chain_length = chain_length;

  ++(ht->hash_n_keys);
  if ((double) ht->hash_n_keys / ht->hash_table_size > 0.80)
    {
      unsigned int new_size;
      new_size = near_prime (2 * ht->hash_table_size);
      *failed = hash_rehash (ht, new_size);
    }

#ifdef TESTING
  assert (hash_table_ok (ht));
#endif

  return NULL;
}

/* If E is already in the table, remove it and return a pointer to
   the just-deleted key (the user may want to deallocate its storage).
   If E is not in the table, don't modify the table and return NULL. */

void *
hash_delete_if_present (HT *ht, const void *e)
{
  HASH_ENT *ent;
  void *key;
  unsigned int idx;
  unsigned int chain_length;

  ent = hash_find_entry (ht, e, &idx, &chain_length, 1);
  if (ent == NULL)
    return NULL;

  if (ent->next == NULL && chain_length == 1)
    --(ht->hash_n_slots_used);

  key = ent->key;

  --(ht->hash_n_keys);
  ht->hash_dirty_max_chain_length = 1;
  if (ent->next == NULL && chain_length < ht->hash_max_chain_length)
    ht->hash_dirty_max_chain_length = 0;

  hash_free_entry (ht, ent);

#ifdef TESTING
  assert (hash_table_ok (ht));
#endif
  return key;
}

void
hash_print_statistics (const HT *ht, FILE *stream)
{
  unsigned int n_slots_used;
  unsigned int n_keys;
  unsigned int max_chain_length;
  int err;

  err = hash_get_statistics (ht, &n_slots_used, &n_keys, &max_chain_length);
  assert (err == 0);
  fprintf (stream, "table size: %d\n", ht->hash_table_size);
  fprintf (stream, "# slots used: %u (%.2f%%)\n", n_slots_used,
	   (100.0 * n_slots_used) / ht->hash_table_size);
  fprintf (stream, "# keys: %u\n", n_keys);
  fprintf (stream, "max chain length: %u\n", max_chain_length);
}

/* If there is *NO* table (so, no meaningful stats) return non-zero
   and don't reference the argument pointers.  Otherwise compute the
   performance statistics and return non-zero. */

int
hash_get_statistics (const HT *ht,
		     unsigned int *n_slots_used,
		     unsigned int *n_keys,
		     unsigned int *max_chain_length)
{
  unsigned int i;

  if (ht == NULL || ht->hash_table == NULL)
    return 1;

  *max_chain_length = 0;
  *n_slots_used = 0;
  *n_keys = 0;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      unsigned int chain_length = 0;
      HASH_ENT *p;

      p = BUCKET_HEAD (ht, i);
      if (p != NULL)
	++(*n_slots_used);

      for (; p; p = p->next)
	++chain_length;

      *n_keys += chain_length;
      if (chain_length > *max_chain_length)
	*max_chain_length = chain_length;
    }
  return 0;
}

int
hash_table_ok (HT *ht)
{
  int code;
  unsigned int n_slots_used;
  unsigned int n_keys;
  unsigned int max_chain_length;

  if (ht == NULL || ht->hash_table == NULL)
    return 1;

  code = hash_get_statistics (ht, &n_slots_used, &n_keys,
			      &max_chain_length);

  if (code != 0
      || n_slots_used != ht->hash_n_slots_used
      || n_keys != ht->hash_n_keys
      || max_chain_length != hash_get_max_chain_length (ht))
    return 0;

  return 1;
}

/* See hash_do_for_each_2 (below) for a variant.  */

void
hash_do_for_each (HT *ht, void (*f) (void *e, void *aux), void *aux)
{
  unsigned int i;

#ifdef TESTING
  assert (hash_table_ok (ht));
#endif

  if (ht->hash_table == NULL)
    return;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *p;
      for (p = BUCKET_HEAD (ht, i); p; p = p->next)
	{
	  (*f) (p->key, aux);
	}
    }
}

/* Just like hash_do_for_each, except that function F returns an int
   that can signal (when non-zero) we should return early.  */

int
hash_do_for_each_2 (HT *ht, int (*f) (void *e, void *aux), void *aux)
{
  unsigned int i;

#ifdef TESTING
  assert (hash_table_ok (ht));
#endif

  if (ht->hash_table == NULL)
    return 0;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *p;
      for (p = BUCKET_HEAD (ht, i); p; p = p->next)
	{
	  int return_code;

	  return_code = (*f) (p->key, aux);
	  if (return_code != 0)
	    return return_code;
	}
    }
  return 0;
}

/* For each entry in the bucket addressed by BUCKET_KEY of the hash
   table HT, invoke the function F.  If F returns non-zero, stop
   iterating and return that value.  Otherwise, apply F to all entries
   in the selected bucket and return zero.  The AUX argument to this
   function is passed as the last argument in each invocation of F.
   The first argument to F is BUCKET_KEY, and the second is the key of
   an entry in the selected bucket. */

int
hash_do_for_each_in_selected_bucket (HT *ht, const void *bucket_key,
				     int (*f) (const void *bucket_key,
					       void *e, void *aux),
				     void *aux)
{
  int idx;
  HASH_ENT *p;

#ifdef TESTING
  assert (hash_table_ok (ht));
#endif

  if (ht->hash_table == NULL)
    return 0;

  idx = ht->hash_hash (bucket_key, ht->hash_table_size);

  for (p = BUCKET_HEAD (ht, idx); p != NULL; p = p->next)
    {
      int return_code;

      return_code = (*f) (bucket_key, p->key, aux);
      if (return_code != 0)
	return return_code;
    }

  return 0;
}

/* Make all buckets empty, placing any chained entries on the free list.
   As with hash_free, apply the user-specified function key_freer
   (if it's not NULL) to the keys of any affected entries. */

void
hash_clear (HT *ht)
{
  unsigned int i;
  HASH_ENT *p;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *tail = NULL;
      HASH_ENT *head = BUCKET_HEAD (ht, i);

      /* Free any keys and get tail pointer to last entry in chain. */
      for (p = head; p; p = p->next)
	{
	  if (ht->hash_key_freer != NULL)
	    ht->hash_key_freer (p->key);
	  p->key = NULL;	/* Make sure no one tries to use this key later. */
	  tail = p;
	}
      BUCKET_HEAD (ht, i) = NULL;

      /* If there's a chain in this bucket, tack it onto the
         beginning of the free list. */
      if (head != NULL)
	{
	  assert (tail != NULL && tail->next == NULL);
	  tail->next = ht->hash_free_entry_list;
	  ht->hash_free_entry_list = head;
	}
    }
  ht->hash_n_slots_used = 0;
  ht->hash_max_chain_length = 0;
  ht->hash_n_keys = 0;
  ht->hash_dirty_max_chain_length = 0;
}

/* Free all storage associated with HT that functions in this package
   have allocated.  If a key_freer function has been supplied (when HT
   was created), this function applies it to the key of each entry before
   freeing that entry. */

static void
hash_free_0 (HT *ht, int free_user_data)
{
  if (free_user_data && ht->hash_key_freer != NULL)
    {
      unsigned int i;

      for (i = 0; i < ht->hash_table_size; i++)
	{
	  HASH_ENT *p;
	  HASH_ENT *next;

	  for (p = BUCKET_HEAD (ht, i); p; p = next)
	    {
	      next = p->next;
	      ht->hash_key_freer (p->key);
	    }
	}
    }

#ifdef USE_OBSTACK
  obstack_free (&(ht->ht_obstack), NULL);
#else
  {
    unsigned int i;
    for (i = 0; i < ht->hash_table_size; i++)
      {
	HASH_ENT *p;
	HASH_ENT *next;

	for (p = BUCKET_HEAD (ht, i); p; p = next)
	  {
	    next = p->next;
	    free (p);
	  }
      }
  }
#endif
  ht->hash_free_entry_list = NULL;
  free (ht->hash_table);
}

void
hash_free (HT *ht)
{
  hash_free_0 (ht, 1);
  free (ht);
}

#ifdef TESTING

void
hash_print (const HT *ht)
{
  int i;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *p;

      if (BUCKET_HEAD (ht, i) != NULL)
	printf ("%d:\n", i);

      for (p = BUCKET_HEAD (ht, i); p; p = p->next)
	{
	  char *s = (char *) p->key;
	  /* FIXME */
	  printf ("  %s\n", s);
	}
    }
}

#endif /* TESTING */

void
hash_get_key_list (const HT *ht, unsigned int bufsize, void **buf)
{
  unsigned int i;
  unsigned int c = 0;

  for (i = 0; i < ht->hash_table_size; i++)
    {
      HASH_ENT *p;

      for (p = BUCKET_HEAD (ht, i); p; p = p->next)
	{
	  if (c >= bufsize)
	    return;
	  buf[c++] = p->key;
	}
    }
}

/* Return the first key in the table.  If the table is empty, return NULL.  */

void *
hash_get_first (const HT *ht)
{
  unsigned int idx;
  HASH_ENT *p;

  if (ht->hash_n_keys == 0)
    return NULL;

  for (idx = 0; idx < ht->hash_table_size; idx++)
    {
      if ((p = BUCKET_HEAD (ht, idx)) != NULL)
	return p->key;
    }
  abort ();
}

/* Return the key in the entry following the entry whose key matches E.
   If there is the only one key in the table and that key matches E,
   return the matching key.  If E is not in the table, return NULL.  */

void *
hash_get_next (const HT *ht, const void *e)
{
  unsigned int idx;
  HASH_ENT *p;

  idx = ht->hash_hash (e, ht->hash_table_size);
  assert (idx < ht->hash_table_size);
  for (p = BUCKET_HEAD (ht, idx); p != NULL; p = p->next)
    {
      if (ht->hash_key_comparator (e, p->key) == 0)
	{
	  if (p->next != NULL)
	    {
	      return p->next->key;
	    }
	  else
	    {
	      unsigned int bucket;

	      /* E is the last or only key in the bucket chain.  */
	      if (ht->hash_n_keys == 1)
		{
		  /* There is only one key in the table, and it matches E.  */
		  return p->key;
		}
	      bucket = idx;
	      do
		{
		  idx = (idx + 1) % ht->hash_table_size;
		  if ((p = BUCKET_HEAD (ht, idx)) != NULL)
		    return p->key;
		}
	      while (idx != bucket);
	    }
	}
    }

  /* E is not in the table.  */
  return NULL;
}
