#serial 1

dnl From Paul Eggert

AC_DEFUN(jm_FUNC_MBRTOWC,
[
  AC_MSG_CHECKING([whether mbrtowc and mbstate_t are properly declared])
  AC_CACHE_VAL(jm_cv_func_mbrtowc,
    [AC_TRY_LINK(
       [#include <wchar.h>],
       [mbstate_t state; return ! (sizeof state && mbrtowc);],
       [jm_cv_func_mbrtowc=yes],
       [jm_cv_func_mbrtowc=no])])
  if test $jm_cv_func_mbrtowc = yes; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_MBRTOWC, 1,
      [Define to 1 if mbrtowc and mbstate_t are properly declared.])
  fi
])
