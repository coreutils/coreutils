#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifndef AT_FDCWD
/* FIXME: use the same value Solaris uses */
# define AT_FDCWD -999

# ifdef __OPENAT_PREFIX
#  undef openat
#  define __OPENAT_CONCAT(x, y) x ## y
#  define __OPENAT_XCONCAT(x, y) __OPENAT_CONCAT (x, y)
#  define __OPENAT_ID(y) __OPENAT_XCONCAT (__OPENAT_PREFIX, y)
#  define openat __OPENAT_ID (openat)
/* FIXME: use proper prototype */
#if 0
   int openat (int fd, char const *filename, int flags, /* mode_t mode */ ...);
#endif
int openat (int fd, char const *filename, int flags);
# endif
#endif
