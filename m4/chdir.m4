#serial 1

# From Jim Meyering.
# Use Gnulib's robust replacement chdir function.
# It can handle arbitrarily long directory names, which means
# that when it is given the name of an existing directory, it
# never fails with ENAMETOOLONG.
#
# FIXME: we don't really want to use this function on systems that
# do not define PATH_MAX...
#
# It still fails with ENAMETOOLONG if the specified directory
# name contains a component that would provoke such a failure
# all by itself (e.g. if the component is longer than PATH_MAX
# on systems that define PATH_MAX).

AC_DEFUN([gl_FUNC_CHDIR],
[
  AC_LIBOBJ([chdir])
  AC_DEFINE([__CHDIR_PREFIX], [[rpl_]],
    [Define to rpl_ if the chdir replacement function should be used.])
  gl_PREREQ_CHDIR
])

AC_DEFUN([gl_PREREQ_CHDIR],
[
  AM_STDBOOL_H
  gl_FUNC_MEMPCPY
  gl_FUNC_OPENAT
])
