#serial 8

dnl From Jim Meyering.
dnl Determine whether chown accepts arguments of -1 for uid and gid.
dnl If it doesn't, arrange to use the replacement function.
dnl

AC_DEFUN([jm_FUNC_CHOWN],
[
  AC_REQUIRE([AC_TYPE_UID_T])dnl
  AC_REQUIRE([AC_FUNC_CHOWN])
  if test $ac_cv_func_chown_works = no; then
    AC_LIBOBJ(chown)
    AC_DEFINE(chown, rpl_chown,
      [Define to rpl_chown if the replacement function should be used.])
    gl_PREREQ_CHOWN
  fi
])

# Prerequisites of lib/chown.c.
AC_DEFUN([gl_PREREQ_CHOWN],
[
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
