#serial 2
dnl Cloned from xstrtoumax.m4.  Keep these files in sync.

# autoconf tests required for use of xstrtoimax.c

AC_DEFUN([jm_AC_PREREQ_XSTRTOIMAX],
[
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_REQUIRE([jm_AC_TYPE_UNSIGNED_LONG_LONG])
  AC_CHECK_DECLS([strtol, strtoul, strtoll, strtoimax, strtoumax])
  AC_CHECK_HEADERS(limits.h stdlib.h inttypes.h)

  AC_CACHE_CHECK([whether <inttypes.h> defines strtoimax as a macro],
    jm_cv_func_strtoimax_macro,
    AC_EGREP_CPP([inttypes_h_defines_strtoimax], [#include <inttypes.h>
#ifdef strtoimax
 inttypes_h_defines_strtoimax
#endif],
      jm_cv_func_strtoimax_macro=yes,
      jm_cv_func_strtoimax_macro=no))

  if test "$jm_cv_func_strtoimax_macro" != yes; then
    AC_REPLACE_FUNCS(strtoimax)
  fi

  dnl Only the replacement strtoimax invokes strtol and strtoll,
  dnl so we need the replacements only if strtoimax does not exist.
  case "$jm_cv_func_strtoimax_macro,$ac_cv_func_strtoimax" in
    no,no)
      AC_REPLACE_FUNCS(strtol)

      dnl We don't need (and can't compile) the replacement strtoll
      dnl unless the type `long long' exists.
      if test "$ac_cv_type_long_long" = yes; then
	AC_REPLACE_FUNCS(strtoll)
      fi
      ;;
  esac
])
