#serial 22

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN([gl_CHECK_DECLS],
[
  AC_REQUIRE([_gl_DECL_HEADERS])
  AC_REQUIRE([AC_HEADER_TIME])
  headers='
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UTMP_H
# include <utmp.h>
#endif

#if HAVE_GRP_H
# include <grp.h>
#endif

#if HAVE_PWD_H
# include <pwd.h>
#endif
'

  AC_CHECK_DECLS([
    free,
    getenv,
    geteuid,
    getgrgid,
    getlogin,
    getpwuid,
    getuid,
    getutent,
    lseek,
    malloc,
    memchr,
    memrchr,
    nanosleep,
    realloc,
    stpcpy,
    strndup,
    strnlen,
    strstr,
    strtoul,
    strtoull,
    ttyname], , , $headers)
])

dnl FIXME: when autoconf has support for it.
dnl This is a little helper so we can require these header checks.
AC_DEFUN([_gl_DECL_HEADERS],
[
  AC_CHECK_HEADERS_ONCE(grp.h pwd.h sys/time.h utmp.h)
])
