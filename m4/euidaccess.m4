# euidaccess.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_NONREENTRANT_EUIDACCESS],
[
  AC_REQUIRE([gl_FUNC_EUIDACCESS])
  AC_DEFINE([PREFER_NONREENTRANT_EUIDACCESS], 1,
    [Define this if you prefer euidaccess to return the correct result
     even if this would make it nonreentrant.  Define this only if your
     entire application is safe even if the uid or gid might temporarily
     change.  If your application uses signal handlers or threads it
     is probably not safe.])
])

AC_DEFUN([gl_FUNC_EUIDACCESS],
[
  dnl Persuade glibc <unistd.h> to declare euidaccess().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_CHECK_DECLS_ONCE([euidaccess])
  AC_REPLACE_FUNCS(euidaccess)
  if test $ac_cv_func_euidaccess = no; then
    gl_PREREQ_EUIDACCESS
  fi
])

# Prerequisites of lib/euidaccess.c.
AC_DEFUN([gl_PREREQ_EUIDACCESS], [
  AC_CHECK_HEADERS_ONCE(libgen.h)
  AC_CHECK_DECLS_ONCE(setregid)
  AC_REQUIRE([AC_FUNC_GETGROUPS])

  # Solaris 9 needs -lgen to get the eaccess function.
  # Save and restore LIBS so -lgen isn't added to it.  Otherwise, *all*
  # programs in the package would end up linked with that potentially-shared
  # library, inducing unnecessary run-time overhead.
  gl_saved_libs=$LIBS
    AC_SEARCH_LIBS(eaccess, [gen],
                   [test "$ac_cv_search_eaccess" = "none required" ||
                    LIB_EACCESS=$ac_cv_search_eaccess])
    AC_SUBST(LIB_EACCESS)
    AC_CHECK_FUNCS(eaccess)
  LIBS=$gl_saved_libs
])
