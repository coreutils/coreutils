#serial 1

dnl From Paul Eggert.

# Define uintmax_t to `unsigned long' or `unsigned long long'
# if <inttypes.h> does not exist.

AC_DEFUN(jm_AC_TYPE_UINTMAX_T,
[
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  if test $jm_ac_cv_header_inttypes_h = no; then
    AC_CACHE_CHECK([for unsigned long long], ac_cv_type_unsigned_long_long,
    [AC_TRY_COMPILE([],
      [unsigned long long i = (unsigned long long) -1;],
      ac_cv_type_unsigned_long_long=yes,
      ac_cv_type_unsigned_long_long=no)])
    if test $ac_cv_type_unsigned_long_long = yes; then
      AC_DEFINE(uintmax_t, unsigned long long)
    else
      AC_DEFINE(uintmax_t, unsigned long)
    fi
  fi
])
