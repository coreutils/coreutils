#serial 3
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FTS],
[
  AC_LIBSOURCES([fts.c, fts_.h])

  dnl Use this version of fts unconditionally, since the GNU libc and
  dnl NetBSD versions have bugs and/or unnecessary limitations.
  AC_LIBOBJ([fts])

  dnl Prerequisites of lib/fts.c.

  # Checks for header files.
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_CHECK_HEADERS_ONCE([inttypes.h stdint.h])
  AC_CHECK_HEADERS_ONCE([sys/param.h])

  # Checks for typedefs, structures, and compiler characteristics.
  AC_REQUIRE([gt_INTTYPES_PRI])

  # Checks for library functions.
  AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
])
