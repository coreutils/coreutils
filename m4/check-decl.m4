#serial 18

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN([jm_CHECK_DECLS],
[
  AC_REQUIRE([_jm_DECL_HEADERS])
  AC_REQUIRE([AC_HEADER_TIME])
  headers='
#include <stdio.h>
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

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
AC_DEFUN([_jm_DECL_HEADERS],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(grp.h memory.h pwd.h string.h strings.h stdlib.h \
                   unistd.h sys/time.h utmp.h utmpx.h)
])
