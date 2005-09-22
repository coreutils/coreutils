#serial 8

# See if we need to emulate a missing ftruncate function using fcntl or chsize.

# Copyright (C) 2000, 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FTRUNCATE],
[
  AC_REPLACE_FUNCS(ftruncate)
  if test $ac_cv_func_ftruncate = no; then
    gl_PREREQ_FTRUNCATE
  fi
])

# Prerequisites of lib/ftruncate.c.
AC_DEFUN([gl_PREREQ_FTRUNCATE],
[
  AC_CHECK_FUNCS(chsize)
])
