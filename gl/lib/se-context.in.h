#ifndef SELINUX_CONTEXT_H
# define SELINUX_CONTEXT_H

# include <errno.h>
/* Some systems don't have ENOTSUP.  */
# ifndef ENOTSUP
#  ifdef ENOSYS
#   define ENOTSUP ENOSYS
#  else
/* Some systems don't have ENOSYS either.  */
#   define ENOTSUP EINVAL
#  endif
# endif

typedef int context_t;
static inline context_t context_new (char const *s)
  { errno = ENOTSUP; return 0; }
static inline char *context_str (context_t con)
  { errno = ENOTSUP; return (void *) 0; }
static inline void context_free (context_t c) {}

static inline int context_user_set (context_t sc, char const *s)
  { errno = ENOTSUP; return -1; }
static inline int context_role_set (context_t sc, char const *s)
  { errno = ENOTSUP; return -1; }
static inline int context_range_set (context_t sc, char const *s)
  { errno = ENOTSUP; return -1; }
static inline int context_type_set (context_t sc, char const *s)
  { errno = ENOTSUP; return -1; }

#endif
