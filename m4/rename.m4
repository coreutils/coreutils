#serial 1

dnl From Volker Borchert.
dnl Determine whether rename works for source paths with a trailing slash.
dnl The rename from SunOS 4.1.1_U1 doesn't.
dnl
dnl If it doesn't, then define RENAME_TRAILING_SLASH_BUG and arrange
dnl to compile the wrapper function.
dnl

AC_DEFUN(vb_FUNC_RENAME,
[
 AC_CACHE_CHECK([whether rename is broken],
  vb_cv_func_rename_trailing_slash_bug,
  [AC_TRY_RUN([
#   include <stdio.h>

    int
    main ()
    {
      if (mkdir ("foo") < 0)
        exit (1);
      if (rename ("foo/", "bar") < 0)
        { rmdir ("foo"); exit (1); }
      else
        { rmdir ("bar"); exit (0); }
    }
	  ],
	 vb_cv_func_rename_trailing_slash_bug=no,
	 vb_cv_func_rename_trailing_slash_bug=yes,
	 dnl When crosscompiling, assume rename is broken.
	 vb_cv_func_rename_trailing_slash_bug=yes)
  ])
  if test $vb_cv_func_rename_trailing_slash_bug = yes; then
    AC_LIBOBJ(rename)
    AC_DEFINE_UNQUOTED(RENAME_TRAILING_SLASH_BUG, 1,
[Define if rename does not work for source paths with a trailing slash,
   like the one from SunOS 4.1.1_U1. ])
  fi
])
