#serial 6
dnl Copyright (C) 2002, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_UNISTD_SAFER],
[
  AC_LIBSOURCES([dup-safer.c, fd-safer.c, pipe-safer.c, unistd-safer.h, unistd--.h])
  AC_LIBOBJ([dup-safer])
  AC_LIBOBJ([fd-safer])
  AC_LIBOBJ([pipe-safer])

  gl_PREREQ_DUP_SAFER
  gl_PREREQ_FD_SAFER
])

# Prerequisites of lib/dup-safer.c.
AC_DEFUN([gl_PREREQ_DUP_SAFER], [
  AC_CHECK_HEADERS_ONCE(unistd.h)
])

# Prerequisites of lib/fd-safer.c.
AC_DEFUN([gl_PREREQ_FD_SAFER], [
  AC_CHECK_HEADERS_ONCE(unistd.h)
])
