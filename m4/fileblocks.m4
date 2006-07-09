# fileblocks.m4 serial 4
dnl Copyright (C) 2002, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FILEBLOCKS],
[
  AC_STRUCT_ST_BLOCKS
  dnl Note: AC_STRUCT_ST_BLOCKS does AC_LIBOBJ(fileblocks).
  if test $ac_cv_member_struct_stat_st_blocks = no; then
    gl_PREREQ_FILEBLOCKS
  fi
])

# Prerequisites of lib/fileblocks.c.
AC_DEFUN([gl_PREREQ_FILEBLOCKS], [
  AC_CHECK_HEADERS_ONCE(sys/param.h)
  :
])
