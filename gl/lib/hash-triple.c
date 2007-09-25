#include <config.h>

#include "hash-triple.h"

#include <stdlib.h>

#include "hash-pjw.h"
#include "same.h"
#include "same-inode.h"

/* Hash an F_triple, and *do* consider the file name.  */
size_t
triple_hash (void const *x, size_t table_size)
{
  struct F_triple const *p = x;
  size_t tmp = hash_pjw (p->name, table_size);

  /* Ignoring the device number here should be fine.  */
  return (tmp ^ p->st_ino) % table_size;
}

/* Hash an F_triple, without considering the file name.  */
size_t
triple_hash_no_name (void const *x, size_t table_size)
{
  struct F_triple const *p = x;

  /* Ignoring the device number here should be fine.  */
  return p->st_ino % table_size;
}

/* Compare two F_triple structs.  */
bool
triple_compare (void const *x, void const *y)
{
  struct F_triple const *a = x;
  struct F_triple const *b = y;
  return (SAME_INODE (*a, *b) && same_name (a->name, b->name)) ? true : false;
}

/* Free an F_triple.  */
void
triple_free (void *x)
{
  struct F_triple *a = x;
  free (a->name);
  free (a);
}
