#serial 1

dnl Just like AC_CHECK_TYPE from autoconf-2.12, but also checks in unistd.h
dnl on systems that have it.  Fujitsu UXP/V needs this for ssize_t.

undefine([AC_CHECK_TYPE])
dnl AC_CHECK_TYPE(TYPE, DEFAULT)
AC_DEFUN(AC_CHECK_TYPE,
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_CHECK_HEADERS(unistd.h)
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(ac_cv_type_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif], ac_cv_type_$1=yes, ac_cv_type_$1=no)])dnl
AC_MSG_RESULT($ac_cv_type_$1)
if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1, $2)
fi
])
