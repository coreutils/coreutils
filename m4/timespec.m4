#serial 1

dnl From Jim Meyering

dnl Define HAVE_STRUCT_TIMESPEC if `struct timespec' is declared in time.h.

AC_DEFUN(jm_CHECK_TYPE_STRUCT_TIMESPEC,
[
  AC_CACHE_CHECK([for struct timespec], fu_cv_sys_struct_timespec,
    [AC_TRY_COMPILE(
      [
#include <time.h>
      ],
      [static struct timespec x; x.tv_sec = x.tv_nsec;],
      fu_cv_sys_struct_timespec=yes,
      fu_cv_sys_struct_timespec=no)
    ])

  if test $fu_cv_sys_struct_timespec = yes; then
    AC_DEFINE_UNQUOTED(HAVE_STRUCT_TIMESPEC, 1,
		       [Define if struct timespec is declared in <time.h>. ])
  fi
])
