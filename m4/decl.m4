#serial 3

AC_DEFUN(jm_CHECK_DECLARATION,
[
  AC_REQUIRE([AC_HEADER_STDC])dnl
  test -z "$ac_cv_header_memory_h" && AC_CHECK_HEADERS(memory.h)
  test -z "$ac_cv_header_string_h" && AC_CHECK_HEADERS(string.h)
  test -z "$ac_cv_header_strings_h" && AC_CHECK_HEADERS(strings.h)
  test -z "$ac_cv_header_stdlib_h" && AC_CHECK_HEADERS(stdlib.h)
  test -z "$ac_cv_header_unistd_h" && AC_CHECK_HEADERS(unistd.h)
  AC_MSG_CHECKING([whether $1 is declared])
  AC_CACHE_VAL(jm_cv_func_decl_$1,
    [AC_TRY_COMPILE($2,
      [
#ifndef $1
char *(*pfn) = (char *(*)) $1
#endif
      ],
      eval "jm_cv_func_decl_$1=yes",
      eval "jm_cv_func_decl_$1=no")])

  if eval "test \"`echo '$jm_cv_func_decl_'$1`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$3], , :, [$3])
  else
    AC_MSG_RESULT(no)
    ifelse([$4], , , [$4
])dnl
  fi
])dnl

dnl jm_CHECK_DECLARATIONS(INCLUDES, FUNCTION... [, ACTION-IF-DECLARED
dnl                       [, ACTION-IF-NOT-DECLARED]])
AC_DEFUN(jm_CHECK_DECLARATIONS,
[
  for jm_func in $2
  do
    jm_CHECK_DECLARATION($jm_func, $1,
    [
      jm_tr_func=HAVE_DECL_`echo $jm_func | tr abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ`
      AC_DEFINE_UNQUOTED($jm_tr_func) $3], $4)dnl
  done
])
