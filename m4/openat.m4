#serial 1

# Written by Jim Meyering.
# See if we need to use our replacement for Solaris' openat function.

AC_DEFUN([gl_FUNC_OPENAT],
[
  AC_REPLACE_FUNCS(openat)
  case $ac_cv_func_openat in
  yes) ;;
  *)
    AC_DEFINE([__OPENAT_PREFIX], [[rpl_]],
      [Define to rpl_ if the openat replacement function should be used.])
    gl_PREREQ_OPENAT;;
  esac
])

AC_DEFUN([gl_PREREQ_OPENAT],
[
  gl_SAVE_CWD
])
