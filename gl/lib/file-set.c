#include <config.h>
#include "file-set.h"

#include "hash-triple.h"
#include "xalloc.h"

/* Record destination file, FILE, and dev/ino from *STATS,
   in the hash table, HT.  If HT is NULL, return immediately.
   If memory allocation fails, exit immediately.  */
void
record_file (Hash_table *ht, char const *file, struct stat const *stats)
{
  struct F_triple *ent;

  if (ht == NULL)
    return;

  ent = xmalloc (sizeof *ent);
  ent->name = xstrdup (file);
  ent->st_ino = stats->st_ino;
  ent->st_dev = stats->st_dev;

  {
    struct F_triple *ent_from_table = hash_insert (ht, ent);
    if (ent_from_table == NULL)
      {
	/* Insertion failed due to lack of memory.  */
	xalloc_die ();
      }

    if (ent_from_table != ent)
      {
	/* There was alread a matching entry in the table, so ENT was
	   not inserted.  Free it.  */
	triple_free (ent);
      }
  }
}

/* Return true if there is an entry in hash table, HT,
   for the file described by FILE and STATS.  */
bool
seen_file (Hash_table const *ht, char const *file,
	   struct stat const *stats)
{
  struct F_triple new_ent;

  if (ht == NULL)
    return false;

  new_ent.name = (char *) file;
  new_ent.st_ino = stats->st_ino;
  new_ent.st_dev = stats->st_dev;

  return !!hash_lookup (ht, &new_ent);
}
