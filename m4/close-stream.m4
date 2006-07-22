dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CLOSE_STREAM],
[
  AC_LIBSOURCES([close-stream.c, close-stream.h])
  AC_LIBOBJ([close-stream])

  dnl Prerequisites of lib/close-stream.c.
  :
])
