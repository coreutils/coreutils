#include <config.h>

#include "hash-triple.h"

#include <stdlib.h>

#include "hash-pjw.h"
#include "same.h"
#include "same-inode.h"

/* Hash an F_triple.  */
size_t
triple_hash (void const *x, size_t table_size)
{
  struct F_triple const *p = x;

  /* Also take the name into account, so that when moving N hard links to the
     same file (all listed on the command line) all into the same directory,
     we don't experience any N^2 behavior.  */
  /* FIXME-maybe: is it worth the overhead of doing this
     just to avoid N^2 in such an unusual case?  N would have
     to be very large to make the N^2 factor noticable, and
     one would probably encounter a limit on the length of
     a command line before it became a problem.  */
  size_t tmp = hash_pjw (p->name, table_size);

  /* Ignoring the device number here should be fine.  */
  return (tmp | p->st_ino) % table_size;
}

/* Hash an F_triple.  */
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
