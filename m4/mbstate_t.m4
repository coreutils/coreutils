# serial 9

# From Paul Eggert.

# BeOS 5 has <wchar.h> but does not define mbstate_t,
# so you can't declare an object of that type.
# Check for this incompatibility with Standard C.

# Include stdlib.h first, because otherwise this test would fail on Linux
# (at least glibc-2.1.3) because the "_XOPEN_SOURCE 500" definition elicits
# a syntax error in wchar.h due to the use of undefined __int32_t.

AC_DEFUN([AC_MBSTATE_T],
  [
   AC_CHECK_HEADERS(stdlib.h)

   AC_CACHE_CHECK([for mbstate_t], ac_cv_type_mbstate_t,
    [AC_TRY_COMPILE([
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <wchar.h>],
      [mbstate_t x; return sizeof x;],
      ac_cv_type_mbstate_t=yes,
      ac_cv_type_mbstate_t=no)])
   if test $ac_cv_type_mbstate_t = no; then
     AC_DEFINE(mbstate_t, int,
	       [Define to a type if <wchar.h> does not define.])
   fi])
