AC_DEFUN(jm_CHECK_DECLARATION,
[
test -z "$ac_cv_header_strings_h" && AC_CHECK_HEADERS(strings.h)
test -z "$ac_cv_header_stdlib_h" && AC_CHECK_HEADERS(stdlib.h)
test -z "$ac_cv_header_unistd_h" && AC_CHECK_HEADERS(unistd.h)
AC_MSG_CHECKING([whether $1 is declared])
AC_CACHE_VAL(jm_cv_func_decl_$1,
[AC_TRY_COMPILE([
#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_STDLIB_H
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
      lower=abcdefghijklmnopqrstuvwxyz
      upper=ABCDEFGHIJKLMNOPQRSTUVWXYZ
      jm_tr_func=HAVE_DECLARATION_`echo $jm_func | tr $lower $upper`
      AC_DEFINE_UNQUOTED($jm_tr_func) $2], $3)dnl
  done
])
