# serial 4

# From Paul Eggert.

# Some versions of BeOS define mbstate_t to be an incomplete type,
# so you can't declare an object of that type.
# Check for this incompatibility with Standard C.

# Include stdlib.h first, because otherwise this test would fail on Linux
# (at least glibc-2.1.3) because the `_XOPEN_SOURCE 500' definition elicits
# a syntax error in wchar.h due to the use of undefined __int32_t.

AC_DEFUN(AC_MBSTATE_T_OBJECT,
  [
   # Check for the mbstate_t type.
   ac_mbs_tmp=$ac_includes_default
   ac_includes_default="
$ac_includes_default
#if HAVE_WCHAR_H
# include <wchar.h>
#endif
"
   AC_CHECK_TYPE(mbstate_t, int)
   # Restore the default value.
   ac_includes_default=$ac_mbs_tmp

   AC_CACHE_CHECK([for mbstate_t object type], ac_cv_type_mbstate_t_object,
    [AC_TRY_COMPILE([
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <wchar.h>],
      [mbstate_t x; return sizeof x;],
      ac_cv_type_mbstate_t_object=yes,
      ac_cv_type_mbstate_t_object=no)])
   if test $ac_cv_type_mbstate_t_object = yes; then
     AC_DEFINE(HAVE_MBSTATE_T_OBJECT, 1,
	       [Define if mbstate_t is an object type.])
   fi])
