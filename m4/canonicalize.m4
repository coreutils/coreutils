#serial 4

# Copyright (C) 2003, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_REQUIRE([AC_HEADER_STDC])
    AC_CHECK_HEADERS(string.h sys/param.h stddef.h)
    AC_CHECK_FUNCS(resolvepath canonicalize_file_name)
    AC_REQUIRE([AC_HEADER_STAT])
  ])
