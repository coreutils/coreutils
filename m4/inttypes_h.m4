#serial 2

dnl From Paul Eggert.

# Define HAVE_INTTYPES_H if <inttypes.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.

AC_DEFUN(jm_AC_HEADER_INTTYPES_H,
[
  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put an entry
    dnl for `HAVE_INTTYPES_H' in config.h.in.
    AC_CHECK_FUNCS(INTTYPES_H)
  fi
  AC_CACHE_CHECK([for inttypes.h], jm_ac_cv_header_inttypes_h,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
#include <inttypes.h>],
    [uintmax_t i = (uintmax_t) -1;],
    jm_ac_cv_header_inttypes_h=yes,
    jm_ac_cv_header_inttypes_h=no)])
  if test $jm_ac_cv_header_inttypes_h = yes; then
    ac_kludge=HAVE_INTTYPES_H
    AC_DEFINE_UNQUOTED($ac_kludge)
  fi
])
