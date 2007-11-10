#ifndef SELINUX_SELINUX_H
# define SELINUX_SELINUX_H

# include <sys/types.h>
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

typedef unsigned short security_class_t;
# define security_context_t char*
# define is_selinux_enabled() 0

static inline int getcon (security_context_t *con) { errno = ENOTSUP; return -1; }
static inline void freecon (security_context_t con) {}


static inline int getfscreatecon (security_context_t *con)
  { errno = ENOTSUP; return -1; }
static inline int setfscreatecon (security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int matchpathcon (char const *s, mode_t m,
				security_context_t *con)
  { errno = ENOTSUP; return -1; }

static inline int getfilecon (char const *s, security_context_t *con)
  { errno = ENOTSUP; return -1; }
static inline int lgetfilecon (char const *s, security_context_t *con)
  { errno = ENOTSUP; return -1; }
static inline int setfilecon (char const *s, security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int lsetfilecon (char const *s, security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int fsetfilecon (int fd, security_context_t con)
  { errno = ENOTSUP; return -1; }

static inline int security_check_context (security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int security_check_context_raw (security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int setexeccon (security_context_t con)
  { errno = ENOTSUP; return -1; }
static inline int security_compute_create (security_context_t scon,
					   security_context_t tcon,
					   security_class_t tclass,
					   security_context_t *newcon)
  { errno = ENOTSUP; return -1; }
static inline int matchpathcon_init_prefix (char const *path,
					    char const *prefix)
  { errno = ENOTSUP; return -1; }
#endif
