#serial 6

dnl From Paul Eggert.

AC_PREREQ(2.13)

# Define HAVE_INTTYPES_H if <inttypes.h> exists,
# doesn't clash with <sys/types.h>, and declares intmax_t and uintmax_t.

AC_DEFUN([jm_AC_HEADER_INTTYPES_H],
[
  AC_CACHE_CHECK([for inttypes.h], jm_ac_cv_header_inttypes_h,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
#include <inttypes.h>],
    [intmax_t i = (intmax_t) -1; uintmax_t ui = (uintmax_t) -1;],
    jm_ac_cv_header_inttypes_h=yes,
    jm_ac_cv_header_inttypes_h=no)])
  if test $jm_ac_cv_header_inttypes_h = yes; then
    AC_DEFINE_UNQUOTED(HAVE_INTTYPES_H, 1,
[Define if <inttypes.h> exists, doesn't clash with <sys/types.h>,
   and declares intmax_t and uintmax_t.])
  fi
])

# Define intmax_t to long or long long if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_INTMAX_T],
[
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  if test $jm_ac_cv_header_inttypes_h = no; then
    AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
    test $ac_cv_type_long_long = yes \
      && ac_type='long long' \
      || ac_type='long'
    AC_DEFINE_UNQUOTED(intmax_t, $ac_type,
      [Define to long or long long if <inttypes.h> doesn't define.])
  fi
])

# Define uintmax_t to unsigned long or unsigned long long
# if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_UINTMAX_T],
[
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  if test $jm_ac_cv_header_inttypes_h = no; then
    AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
    test $ac_cv_type_unsigned_long_long = yes \
      && ac_type='unsigned long long' \
      || ac_type='unsigned long'
    AC_DEFINE_UNQUOTED(uintmax_t, $ac_type,
[Define to unsigned long or unsigned long long
   if <inttypes.h> doesn't define.])
  fi
])
