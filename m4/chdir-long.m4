#serial 6

# Use Gnulib's robust chdir function.
# It can handle arbitrarily long directory names, which means
# that when it is given the name of an existing directory, it
# never fails with ENAMETOOLONG.
# Arrange to compile chdir-long.c only on systems that define PATH_MAX.

dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([gl_FUNC_CHDIR_LONG],
[
  AC_LIBSOURCES([chdir-long.c, chdir-long.h])
  AC_CACHE_CHECK([whether this system has a definition of PATH_MAX],
    gl_have_path_max_definition,
    [AC_EGREP_CPP([have_path_max_definition],
                  [#include <unistd.h>
#include <limits.h>
#ifdef PATH_MAX
have_path_max_definition
#endif],
    gl_have_path_max_definition=yes,
    gl_have_path_max_definition=no)])

  if test $gl_have_path_max_definition = yes; then
    AC_LIBOBJ([chdir-long])
    gl_PREREQ_CHDIR_LONG
  fi
])

AC_DEFUN([gl_PREREQ_CHDIR_LONG],
[
  AM_STDBOOL_H
  gl_FUNC_MEMPCPY
  gl_FUNC_OPENAT
  gl_FUNC_MEMRCHR
])
