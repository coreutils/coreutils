#serial 4

dnl FIXME: put these prerequisite-only *.m4 files in a separate
dnl directory -- otherwise, they'll conflict with existing files.

dnl These are the prerequisite macros for GNU's error.c file.
AC_DEFUN([jm_PREREQ_ERROR],
[
  AC_CHECK_FUNCS(strerror vprintf doprnt)
  AC_CHECK_DECLS([strerror])
  AC_FUNC_STRERROR_R
  AC_HEADER_STDC
])
