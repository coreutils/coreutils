# backupfile.m4 serial 9
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_BACKUPFILE],
[
  AC_LIBSOURCES([backupfile.c, backupfile.h])
  AC_LIBOBJ([backupfile])

  dnl Prerequisites of lib/backupfile.c.
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_DIRENT_D_INO])
  AC_REQUIRE([gl_AC_DOS])
  AC_REQUIRE([AC_SYS_LONG_FILE_NAMES])
  AC_CHECK_FUNCS_ONCE([pathconf])
])
