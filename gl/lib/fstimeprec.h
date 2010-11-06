#include <sys/stat.h>

#ifndef ATTRIBUTE_MALLOC
# if __GNUC__ >= 3
#  define ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
# else
#  define ATTRIBUTE_MALLOC
# endif
#endif

#ifndef _GL_ARG_NONNULL
# if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#  define _GL_ARG_NONNULL(params) __attribute__ ((__nonnull__ params))
# else
#  define _GL_ARG_NONNULL(params)
# endif
#endif

struct fstimeprec *fstimeprec_alloc (void) ATTRIBUTE_MALLOC;
void fstimeprec_free (struct fstimeprec *) _GL_ARG_NONNULL ((1));
int fstimeprec (struct fstimeprec *, struct stat const *)
  _GL_ARG_NONNULL ((2));
