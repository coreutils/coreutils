#include "dev-map.h"

struct di_set_state
{
  /* A map to help compact device numbers.  */
  struct dev_map dev_map;

  /* A set of compact dev,inode pairs.  */
  struct hash_table *di_set;
};

#undef _ATTRIBUTE_NONNULL_
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || 3 < __GNUC__
# define _ATTRIBUTE_NONNULL_(m,...) __attribute__ ((__nonnull__ (m)))
#else
# define _ATTRIBUTE_NONNULL_(m,...)
#endif

int di_set_init (struct di_set_state *, size_t) _ATTRIBUTE_NONNULL_ (1);
void di_set_free (struct di_set_state *) _ATTRIBUTE_NONNULL_ (1);
int di_set_insert (struct di_set_state *, dev_t, ino_t)
  _ATTRIBUTE_NONNULL_ (1);

struct di_ent;
int di_ent_create (struct di_set_state *di_set_state,
                   dev_t dev, ino_t ino,
                   struct di_ent **di_ent)
  _ATTRIBUTE_NONNULL_ (1,4);
