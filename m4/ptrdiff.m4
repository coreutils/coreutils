# From Ulrich Drepper.

# serial 1

AC_DEFUN(AM_TYPE_PTRDIFF_T,
  [AC_CACHE_CHECK([for ptrdiff_t], am_cv_type_ptrdiff_t,
     [AC_TRY_COMPILE([#include <stddef.h>], [ptrdiff_t p],
		     am_cv_type_ptrdiff_t=yes, am_cv_type_ptrdiff_t=no)])
   if test $am_cv_type_ptrdiff_t = yes; then
     AC_DEFINE(HAVE_PTRDIFF_T)
   fi
])
