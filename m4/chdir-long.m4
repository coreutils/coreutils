#serial 1

# Written by Jim Meyering.
# Use Gnulib's robust replacement chdir function.
# It can handle arbitrarily long directory names, which means
# that when it is given the name of an existing directory, it
# never fails with ENAMETOOLONG.
#
# It still fails with ENAMETOOLONG if the specified directory
# name contains a component that would provoke such a failure
# all by itself (e.g. if the component is longer than PATH_MAX
# on systems that define PATH_MAX).

AC_DEFUN([gl_FUNC_CHDIR_LONG],
[
  gl_PREREQ_CHDIR_LONG
])

AC_DEFUN([gl_PREREQ_CHDIR_LONG],
[
  AM_STDBOOL_H
  gl_FUNC_MEMPCPY
  gl_FUNC_OPENAT
])
