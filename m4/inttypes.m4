#serial 6

dnl From Paul Eggert.

AC_PREREQ(2.52)

# Define intmax_t to long or long long if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_INTMAX_T],
[
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_CHECK_TYPE(intmax_t, ,
    [test $ac_cv_type_long_long = yes \
       && ac_type='long long' \
       || ac_type='long'
     AC_DEFINE_UNQUOTED(intmax_t, $ac_type,
       [Define to widest signed type if <inttypes.h> doesn't define.])])
])

# Define uintmax_t to unsigned long or unsigned long long
# if <inttypes.h> doesn't define.

AC_DEFUN([jm_AC_TYPE_UINTMAX_T],
[
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_TYPE(uintmax_t, ,
    [test $ac_cv_type_unsigned_long_long = yes \
       && ac_type='unsigned long long' \
       || ac_type='unsigned long'
     AC_DEFINE_UNQUOTED(uintmax_t, $ac_type,
       [Define to widest unsigned type if <inttypes.h> doesn't define.])])
])
