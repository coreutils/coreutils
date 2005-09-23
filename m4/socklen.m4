# socklen.m4 serial 1
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson.

AC_DEFUN([gl_SOCKLEN_T],
[
  AC_CHECK_HEADERS_ONCE(sys/types.h sys/socket.h)
  AC_CHECK_TYPE([socklen_t],, [AC_DEFINE([socklen_t], [int],
                [Map `socklen_t' to `int' if it is missing.])], [
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif)
])
])
