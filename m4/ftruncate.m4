#serial 1

# See if we need to emulate a missing ftruncate function using fcntl.

AC_DEFUN(jm_FUNC_FTRUNCATE,
[
  AC_CHECK_FUNCS(ftruncate, , [ftruncate_missing=yes])

  if test "$ftruncate_missing" = yes; then
    AC_CHECK_HEADERS(unistd.h)
    AC_MSG_CHECKING([fcntl emulation of ftruncate])
    AC_CACHE_VAL(fu_cv_sys_ftruncate_emulation,
    [AC_TRY_LINK([
#include <sys/types.h>
#include <fcntl.h>], [
#if !defined(F_CHSIZE) && !defined(F_FREESP)
  chsize();
#endif
  ],
      fu_cv_sys_ftruncate_emulation=yes,
      fu_cv_sys_ftruncate_emulation=no)])
    AC_MSG_RESULT($fu_cv_sys_ftruncate_emulation)
    if test $fu_cv_sys_ftruncate_emulation = yes; then
      LIBOBJS="$LIBOBJS ftruncate.$ac_objext"
    fi
  fi
])
