#serial 8

dnl From Jim Meyering.
dnl A wrapper around AC_FUNC_GETGROUPS.

AC_DEFUN([jm_FUNC_GETGROUPS],
[
  AC_REQUIRE([AC_FUNC_GETGROUPS])
  if test $ac_cv_func_getgroups_works = no; then
    AC_LIBOBJ(getgroups)
    AC_DEFINE(getgroups, rpl_getgroups,
      [Define as rpl_getgroups if getgroups doesn't work right.])
    gl_PREREQ_GETGROUPS
  fi
  test -n "$GETGROUPS_LIB" && LIBS="$GETGROUPS_LIB $LIBS"
])

# Prerequisites of lib/getgroups.c.
AC_DEFUN([gl_PREREQ_GETGROUPS],
[
  AC_REQUIRE([AC_TYPE_GETGROUPS])
])
