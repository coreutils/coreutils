#include <sys/types.h>

#undef _ATTRIBUTE_NONNULL_
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || 3 < __GNUC__
# define _ATTRIBUTE_NONNULL_(m) __attribute__ ((__nonnull__ (m)))
#else
# define _ATTRIBUTE_NONNULL_(m)
#endif

struct di_set *di_set_alloc (void);
int di_set_insert (struct di_set *, dev_t, ino_t) _ATTRIBUTE_NONNULL_ (1);
void di_set_free (struct di_set *) _ATTRIBUTE_NONNULL_ (1);
