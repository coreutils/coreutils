#serial 2

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN([jm_FUNC_LCHOWN],
[
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REPLACE_FUNCS(lchown)
])
