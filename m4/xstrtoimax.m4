#serial 1
dnl Cloned from xstrtoumax.m4.  Keep these files in sync.

# autoconf tests required for use of xstrtoimax.c

AC_DEFUN([jm_AC_PREREQ_XSTRTOIMAX],
[
  AC_REQUIRE([jm_AC_TYPE_INTMAX_T])
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])
  AC_CHECK_DECLS([strtol, strtoll])
  AC_CHECK_HEADERS(limits.h stdlib.h)

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

  dnl We don't need (and can't compile) the replacement strtoll
  dnl unless the type `long long' exists.
  dnl Also, only the replacement strtoimax invokes strtoll,
  dnl so we need the replacement strtoll only if strtoimax does not exist.
  case "$ac_cv_type_long_long,$jm_cv_func_strtoimax_macro,$ac_cv_func_strtoimax" in
    yes,no,no)
      AC_REPLACE_FUNCS(strtoll)

      dnl Check for strtol.  Mainly as a cue to cause automake to include
      dnl strtol.c -- that file is included by each of strtoul.c and strtoull.c.
      AC_REPLACE_FUNCS(strtol)
      ;;
  esac

  case "$jm_cv_func_strtoimax_macro,$ac_cv_func_strtoimax" in
    no,no)
      AC_REPLACE_FUNCS(strtoul)

      dnl See explanation above.
      AC_REPLACE_FUNCS(strtol)
      ;;
  esac

])
