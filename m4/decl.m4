AC_DEFUN(jm_CHECK_DECLARATION,
[
  AC_REQUIRE(AC_HEADER_STDC)
  test -z "$ac_cv_header_memory_h" && AC_CHECK_HEADERS(memory.h)
  test -z "$ac_cv_header_string_h" && AC_CHECK_HEADERS(string.h)
  test -z "$ac_cv_header_strings_h" && AC_CHECK_HEADERS(strings.h)
  test -z "$ac_cv_header_unistd_h" && AC_CHECK_HEADERS(unistd.h)
  AC_MSG_CHECKING([whether $1 is declared])
  AC_CACHE_VAL(jm_cv_func_decl_$1,
    [AC_TRY_COMPILE([
#include <stdio.h>
#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif],
[
#ifndef $1
char *(*pfn) = (char *(*)) $1
#endif
],
  eval "jm_cv_func_decl_$1=yes",
  eval "jm_cv_func_decl_$1=no")])

if eval "test \"`echo '$jm_cv_func_decl_'$1`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
  ifelse([$3], , , [$3
])dnl
fi
])dnl

dnl jm_CHECK_DECLARATIONS(FUNCTION... [, ACTION-IF-DECLARED
dnl                       [, ACTION-IF-NOT-DECLARED]])
AC_DEFUN(jm_CHECK_DECLARATIONS,
[
  for jm_func in $1
  do
    jm_CHECK_DECLARATION($jm_func,
    [
      jm_tr_func=HAVE_DECLARATION_`echo $jm_func | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ`
      AC_DEFINE_UNQUOTED($jm_tr_func) $2], $3)dnl
  done
])
