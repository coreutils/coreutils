#serial 3

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN([jm_FUNC_LCHOWN],
[
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REPLACE_FUNCS(lchown)
  if test $ac_cv_func_lchown = no; then
    gl_PREREQ_LCHOWN
  fi
])

# Prerequisites of lib/lchown.c.
AC_DEFUN([gl_PREREQ_LCHOWN],
[
  AC_REQUIRE([AC_HEADER_STAT])
  :
])
