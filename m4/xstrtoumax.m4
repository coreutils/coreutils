#serial 4

# autoconf tests required for use of xstrtoumax.c

AC_DEFUN([jm_AC_PREREQ_XSTRTOUMAX],
[
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_DECLS([strtol, strtoul, strtoull, strtoimax, strtoumax])
  AC_CHECK_HEADERS(limits.h stdlib.h inttypes.h)

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoumax as a macro],
    jm_cv_func_strtoumax_macro,
    AC_EGREP_CPP([inttypes_h_defines_strtoumax], [#include <inttypes.h>
#ifdef strtoumax
 inttypes_h_defines_strtoumax
#endif],
      jm_cv_func_strtoumax_macro=yes,
      jm_cv_func_strtoumax_macro=no))

  if test "$jm_cv_func_strtoumax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoumax)
  fi

  dnl Only the replacement strtoumax invokes strtoul and strtoull,
  dnl so we need the replacements only if strtoumax does not exist.
  case "$jm_cv_func_strtoumax_macro,$ac_cv_func_strtoumax" in
    no,no)
      AC_REPLACE_FUNCS(strtoul)

      dnl We don't need (and can't compile) the replacement strtoull
      dnl unless the type `unsigned long long' exists.
      if test "$ac_cv_type_unsigned_long_long" = yes; then
	AC_REPLACE_FUNCS(strtoull)
      fi
      ;;
  esac
])
