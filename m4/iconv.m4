#serial 1

dnl From Bruno Haible.

AC_DEFUN(jm_ICONV,
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable libiconv installed).
  AC_CACHE_CHECK(for iconv, jm_cv_func_iconv, [
    jm_cv_func_iconv="no, consider installing libiconv"
    jm_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      jm_cv_func_iconv=yes)
    if test "$jm_cv_func_iconv" != yes; then
      jm_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        jm_cv_lib_iconv=yes
        jm_cv_func_iconv=yes)
      LIBS="$jm_save_LIBS"
    fi
  ])
  if test "$jm_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
  fi
  LIBICONV=
  if test "$jm_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])
