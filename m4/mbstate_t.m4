# serial 1

# From Paul Eggert.

# Some versions of BeOS define mbstate_t to be an incomplete type,
# so you can't declare an object of that type.
# Check for this incompatibility with Standard C.

AC_DEFUN(AC_MBSTATE_T_OBJECT,
  [AC_CACHE_CHECK([for mbstate_t object type], ac_cv_type_mbstate_t_object,
    [AC_TRY_COMPILE([#include <wchar.h>],
      [mbstate_t x; return sizeof x;],
      ac_cv_type_mbstate_t_object=yes,
      ac_cv_type_mbstate_t_object=no)])
   if test $ac_cv_type_mbstate_t_object = yes; then
     AC_DEFINE(HAVE_MBSTATE_T_OBJECT, 1,
	       [Define if mbstate_t is an object type.])
   fi])
