#serial 5

dnl From Volker Borchert.
dnl Determine whether rename works for source paths with a trailing slash.
dnl The rename from SunOS 4.1.1_U1 doesn't.
dnl
dnl If it doesn't, then define RENAME_TRAILING_SLASH_BUG and arrange
dnl to compile the wrapper function.
dnl

AC_DEFUN([vb_FUNC_RENAME],
[
 AC_CACHE_CHECK([whether rename is broken],
  vb_cv_func_rename_trailing_slash_bug,
  [
    rm -rf conftest.d1 conftest.d2
    mkdir conftest.d1 ||
      AC_MSG_ERROR([cannot create temporary directory])
    AC_TRY_RUN([
#       include <stdio.h>
        int
        main ()
        {
          exit (rename ("conftest.d1/", "conftest.d2") ? 1 : 0);
        }
      ],
      vb_cv_func_rename_trailing_slash_bug=no,
      vb_cv_func_rename_trailing_slash_bug=yes,
      dnl When crosscompiling, assume rename is broken.
      vb_cv_func_rename_trailing_slash_bug=yes)

      rm -rf conftest.d1 conftest.d2
  ])
  if test $vb_cv_func_rename_trailing_slash_bug = yes; then
    AC_LIBOBJ(rename)
    AC_DEFINE(rename, rpl_rename,
      [Define to rpl_rename if the replacement function should be used.])
    AC_DEFINE(RENAME_TRAILING_SLASH_BUG, 1,
      [Define if rename does not work for source paths with a trailing slash,
       like the one from SunOS 4.1.1_U1.])
    gl_PREREQ_RENAME
  fi
])

# Prerequisites of lib/rename.c.
AC_DEFUN([gl_PREREQ_RENAME],
[
  AC_CHECK_HEADERS_ONCE(stdlib.h string.h)
  AC_CHECK_DECLS_ONCE(free)
])
