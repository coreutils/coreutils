#serial 7

AC_DEFUN([gl_AFS],
  [
    AC_ARG_WITH(afs,
                AC_HELP_STRING([--with-afs],
                               [support for the Andrew File System [[default=no]]]),
    test "$withval" = no || with_afs=yes, with_afs=no)
    if test "$with_afs" = yes; then
      AC_DEFINE(AFS, 1, [Define if you have the Andrew File System.])
    fi
  ])
