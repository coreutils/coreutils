#ifndef _hash_h_
# define _hash_h_ 1

# include <stdio.h>
# include <stdlib.h>
# include <assert.h>

# ifdef USE_OBSTACK
#  include "obstack.h"
# endif
# include "xalloc.h"

# define obstack_chunk_alloc xmalloc
# define obstack_chunk_free free

struct hash_ent
  {
    void *key;
    struct hash_ent *next;
  };
typedef struct hash_ent HASH_ENT;

/* This is particularly useful to cast uses in hash_initialize of the
   system free function.  */
typedef void (*Hash_key_freer_type) (void *key);

struct HT
  {
    /* User-supplied function for freeing keys.  It is specified in
       hash_initialize.  If non-null, it is used by hash_free,
       hash_clear, and hash_rehash.  You should specify `free' here
       only if you want these functions to free all of your `key'
       data.  This is typically the case when your key is simply
       an auxilliary struct that you have malloc'd to aggregate
       several values.   */
    Hash_key_freer_type hash_key_freer;

    /* User-supplied hash function that hashes entry E to an integer
       in the range 0..TABLE_SIZE-1.  */
    unsigned int (*hash_hash) (const void *e, unsigned int table_size);

    /* User-supplied function that determines whether a new entry is
       unique by comparing the new entry to entries that hashed to the
       same bucket index.  It should return zero for a pair of entries
       that compare equal, non-zero otherwise.  */

    int (*hash_key_comparator) (const void *, const void *);

    HASH_ENT **hash_table;
    unsigned int hash_table_size;
    unsigned int hash_n_slots_used;
    unsigned int hash_max_chain_length;

    /* Gets set when an entry is deleted from a chain of length
       hash_max_chain_length.  Indicates that hash_max_chain_length
       may no longer be valid.  */
    unsigned int hash_dirty_max_chain_length;

    /* Sum of lengths of all chains (not counting any dummy
       header entries).  */
    unsigned int hash_n_keys;

    /* A linked list of freed HASH_ENT structs.
       FIXME: Perhaps this is unnecessary and we should simply free
       and reallocate such structs.  */
    HASH_ENT *hash_free_entry_list;

    /* FIXME: comment.  */
# ifdef USE_OBSTACK
    struct obstack ht_obstack;
# endif
  };

typedef struct HT HT;

unsigned int
  hash_get_n_slots_used (const HT *ht);

unsigned int
  hash_get_max_chain_length (HT *ht);

int
  hash_rehash (HT *ht, unsigned int new_table_size);

unsigned int
  hash_get_table_size (const HT *ht);

HT *
  hash_initialize (unsigned int table_size,
		   void (*key_freer) (void *key),
		   unsigned int (*hash) (const void *, unsigned int),
		   int (*equality_tester) (const void *, const void *));

unsigned int
  hash_get_n_keys (const HT *ht);

int
  hash_query_in_table (const HT *ht, const void *e);

void *
  hash_lookup (const HT *ht, const void *e);

void *
  hash_insert_if_absent (HT *ht, const void *e, int *failed);

void *
  hash_delete_if_present (HT *ht, const void *e);

void
  hash_print_statistics (const HT *ht, FILE *stream);

int
  hash_get_statistics (const HT *ht, unsigned int *n_slots_used,
		       unsigned int *n_keys,
		       unsigned int *max_chain_length);

int
  hash_table_ok (HT *ht);

void
  hash_do_for_each (HT *ht, void (*f) (void *e, void *aux), void *aux);

int
  hash_do_for_each_2 (HT *ht, int (*f) (void *e, void *aux), void *aux);

int
  hash_do_for_each_in_selected_bucket (HT *ht, const void *key,
				       int (*f) (const void *bucket_key,
						 void *e, void *aux),
				       void *aux);

void
  hash_clear (HT *ht);

void
  hash_free (HT *ht);

void
  hash_get_key_list (const HT *ht, unsigned int bufsize, void **buf);

void *
  hash_get_first (const HT *ht);

void *
  hash_get_next (const HT *ht, const void *e);

/* This interface to hash_insert_if_absent is used frequently enough to
   merit a macro here.  */

# define HASH_INSERT_NEW_ITEM(ht, item)					\
  do									\
    {									\
      void *already;							\
      int _failed;							\
      already = hash_insert_if_absent ((ht), (item), &_failed);		\
      assert (already == NULL);						\
      assert (!_failed);						\
    }									\
  while (0)

#endif /* _hash_h_ */
