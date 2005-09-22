#serial 9

# Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_LIBSOURCES([canonicalize.c, canonicalize.h])
    AC_LIBOBJ([canonicalize])

    AC_CHECK_HEADERS(sys/param.h)
    AC_CHECK_FUNCS(resolvepath canonicalize_file_name)
  ])
