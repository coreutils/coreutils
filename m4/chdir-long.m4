#serial 2

# Written by Jim Meyering.
# Use Gnulib's robust chdir function.
# It can handle arbitrarily long directory names, which means
# that when it is given the name of an existing directory, it
# never fails with ENAMETOOLONG.

AC_DEFUN([gl_FUNC_CHDIR_LONG],
[
  AC_LIBSOURCES([chdir-long.c, chdir-long.h])
  AC_LIBOBJ([chdir-long])
  gl_PREREQ_CHDIR_LONG
])

AC_DEFUN([gl_PREREQ_CHDIR_LONG],
[
  AM_STDBOOL_H
  gl_FUNC_MEMPCPY
  gl_FUNC_OPENAT
])
