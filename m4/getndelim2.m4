# getndelim2.m4 serial 4
dnl Copyright (C) 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETNDELIM2],
[
  # Avoid multiple inclusions of getndelim2.o into LIBOBJS.
  # This hack won't be needed after gnulib requires Autoconf 2.58 or later.
  case " $LIB@&t@OBJS " in
  *" getndelim2.$ac_objext "* ) ;;
  *) AC_LIBOBJ(getndelim2);;
  esac

  gl_PREREQ_GETNDELIM2
])

# Prerequisites of lib/getndelim2.h and lib/getndelim2.c.
AC_DEFUN([gl_PREREQ_GETNDELIM2],
[
  dnl Prerequisites of lib/getndelim2.h.
  AC_REQUIRE([gt_TYPE_SSIZE_T])
  dnl No prerequisites of lib/getndelim2.c.
])
