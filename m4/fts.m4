#serial 2
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
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS_ONCE([fcntl.h inttypes.h stddef.h stdint.h])
  AC_CHECK_HEADERS_ONCE([stdlib.h string.h sys/param.h unistd.h])

  # Checks for typedefs, structures, and compiler characteristics.
  AC_REQUIRE([AM_STDBOOL_H])
  AC_REQUIRE([AC_C_CONST])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([gt_INTTYPES_PRI])
  AC_CHECK_TYPES([ptrdiff_t])

  # Checks for library functions.
  AC_REQUIRE([AC_FUNC_CLOSEDIR_VOID])
  AC_REQUIRE([AC_FUNC_LSTAT])
  AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
  AC_REQUIRE([AC_FUNC_STAT])
  AC_CHECK_FUNCS_ONCE([fchdir memmove memset strrchr])
])
