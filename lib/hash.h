#ifndef HASH_H
# define HASH_H 1

# if HAVE_CONFIG_H
#  include <config.h>
# endif

# ifndef PARAMS
#  if defined (__GNUC__) || __STDC__
#   define PARAMS(args) args
#  else
#   define PARAMS(args) ()
#  endif
# endif

# include <stdio.h>
# include <assert.h>

# ifdef STDC_HEADERS
#  include <stdlib.h>
# endif

# ifndef HAVE_DECL_FREE
void free ();
# endif

# ifndef HAVE_DECL_MALLOC
char *malloc ();
# endif

# define USE_OBSTACK
# ifdef USE_OBSTACK
#  include "obstack.h"
# endif

# define obstack_chunk_alloc malloc
# define obstack_chunk_free free

struct hash_ent
  {
    void *key;
    struct hash_ent *next;
  };
typedef struct hash_ent HASH_ENT;

/* This is particularly useful to cast uses in hash_initialize of the
   system free function.  */
typedef void (*Hash_key_freer_type) PARAMS((void *key));

struct HT
  {
    /* User-supplied function for freeing keys.  It is specified in
       hash_initialize.  If non-null, it is used by hash_free and
       hash_clear.  You should specify `free' here only if you want
       these functions to free all of your `key' data.  This is typically
       the case when your key is simply an auxilliary struct that you
       have malloc'd to aggregate several values.   */
    Hash_key_freer_type hash_key_freer;

    /* User-supplied hash function that hashes entry E to an integer
       in the range 0..TABLE_SIZE-1.  */
    unsigned int (*hash_hash) PARAMS((const void *e, unsigned int table_size));

    /* User-supplied function that determines whether a new entry is
       unique by comparing the new entry to entries that hashed to the
       same bucket index.  It should return zero for a pair of entries
       that compare equal, non-zero otherwise.  */

    int (*hash_key_comparator) PARAMS((const void *, const void *));

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
  hash_get_n_slots_used PARAMS((const HT *ht));

unsigned int
  hash_get_max_chain_length PARAMS((HT *ht));

int
  hash_rehash PARAMS((HT *ht, unsigned int new_table_size));

unsigned int
  hash_get_table_size PARAMS((const HT *ht));

HT *
  hash_initialize PARAMS((unsigned int table_size,
			  void (*key_freer) PARAMS((void *key)),
			  unsigned int (*hash) PARAMS((const void *,
						       unsigned int)),
			  int (*equality_tester) PARAMS((const void *,
							 const void *))));

unsigned int
  hash_get_n_keys PARAMS((const HT *ht));

int
  hash_query_in_table PARAMS((const HT *ht, const void *e));

void *
  hash_lookup PARAMS((const HT *ht, const void *e));

void *
  hash_insert_if_absent PARAMS((HT *ht,
				const void *e,
				int *failed));

void *
  hash_delete_if_present PARAMS((HT *ht, const void *e));

void
  hash_print_statistics PARAMS((const HT *ht, FILE *stream));

int
  hash_get_statistics PARAMS((const HT *ht, unsigned int *n_slots_used,
			      unsigned int *n_keys,
			      unsigned int *max_chain_length));

int
  hash_table_ok PARAMS((HT *ht));

void
  hash_do_for_each PARAMS((HT *ht,
			   void (*f) PARAMS((void *e, void *aux)),
			   void *aux));

int
  hash_do_for_each_2 PARAMS((HT *ht,
			     int (*f) PARAMS((void *e, void *aux)),
			     void *aux));

int
  hash_do_for_each_in_selected_bucket PARAMS((HT *ht,
					      const void *key,
				      int (*f) PARAMS((const void *bucket_key,
						       void *e,
						       void *aux)),
					      void *aux));

void
  hash_clear PARAMS((HT *ht));

void
  hash_free PARAMS((HT *ht));

void
  hash_get_key_list PARAMS((const HT *ht,
			    unsigned int bufsize,
			    void **buf));

void *
  hash_get_first PARAMS((const HT *ht));

void *
  hash_get_next PARAMS((const HT *ht, const void *e));

/* This interface to hash_insert_if_absent is used frequently enough to
   merit a macro here.  */

# define HASH_INSERT_NEW_ITEM(Ht, Item, Failp)				\
  do									\
    {									\
      void *_already;							\
      _already = hash_insert_if_absent ((Ht), (Item), Failp);		\
      assert (_already == NULL);					\
    }									\
  while (0)

#endif /* HASH_H */
