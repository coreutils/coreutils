#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "hash.h"

struct dev_map
{
  /* KEY,VAL pair, where KEY is the raw st_dev value
     and VAL is the small number that maps to.  */
  struct hash_table *dev_map;
  size_t n_device;
};

#undef _ATTRIBUTE_NONNULL_
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || 3 < __GNUC__
# define _ATTRIBUTE_NONNULL_(m) __attribute__ ((__nonnull__ (m)))
#else
# define _ATTRIBUTE_NONNULL_(m)
#endif

int dev_map_init (struct dev_map *) _ATTRIBUTE_NONNULL_ (1);
void dev_map_free (struct dev_map *) _ATTRIBUTE_NONNULL_ (1);
int dev_map_insert (struct dev_map *, dev_t) _ATTRIBUTE_NONNULL_ (1);
