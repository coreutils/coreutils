#serial 3

dnl From Bruno Haible.

AC_DEFUN(jm_ICONV,
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).
  AC_CACHE_CHECK(for iconv, jm_cv_func_iconv, [
    jm_cv_func_iconv="no, consider installing GNU libiconv"
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
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(jm_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t* outbytesleft);
#else
size_t iconv();
#endif
], [], jm_cv_proto_iconv_arg1="", jm_cv_proto_iconv_arg1="const")
      jm_cv_proto_iconv="extern size_t iconv (iconv_t cd, $jm_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t* outbytesleft);"])
    jm_cv_proto_iconv=`echo "[$]jm_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]jm_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $jm_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$jm_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])
