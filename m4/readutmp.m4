# readutmp.m4 serial 9
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_READUTMP],
[
  AC_LIBSOURCES([readutmp.c, readutmp.h])
  AC_LIBOBJ([readutmp])

  dnl Prerequisites of lib/readutmp.h and lib/readutmp.c.
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([gl_FUNC_FREE])
  AC_CHECK_HEADERS_ONCE(utmp.h utmpx.h)
  AC_CHECK_FUNCS_ONCE(utmpname utmpxname)
  AC_CHECK_DECLS(getutent,,,[
#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif
])
  if test $ac_cv_header_utmp_h = yes || test $ac_cv_header_utmpx_h = yes; then
    utmp_includes="\
$ac_includes_default
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#endif
#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif
"
    AC_CHECK_MEMBERS([struct utmpx.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_id],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_id],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_exit],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_termination],,,[$utmp_includes])
  fi
])
