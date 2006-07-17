# Make a file's ancestor directories.
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MKANCESDIRS],
[
  AC_LIBSOURCES([mkancesdirs.c, mkancesdirs.h])
  AC_LIBOBJ([mkancesdirs])
])
