#include <sys/types.h>

#undef _ATTRIBUTE_NONNULL_
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || 3 < __GNUC__
# define _ATTRIBUTE_NONNULL_(m) __attribute__ ((__nonnull__ (m)))
#else
# define _ATTRIBUTE_NONNULL_(m)
#endif

#define INO_MAP_INSERT_FAILURE ((size_t) -1)

struct ino_map *ino_map_alloc (size_t);
void ino_map_free (struct ino_map *) _ATTRIBUTE_NONNULL_ (1);
size_t ino_map_insert (struct ino_map *, ino_t) _ATTRIBUTE_NONNULL_ (1);
