#serial 7

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(jm_CHECK_DECLS,
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
'

  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_DEFINE(HAVE_DECL_FREE, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_LSEEK, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_MALLOC, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_MEMCHR, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_REALLOC, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STPCPY, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRSTR, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRTOUL, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_STRTOULL, 1, [Define if this function is declared.])
    AC_DEFINE(HAVE_DECL_NANOSLEEP, 1, [Define if this function is declared.])
  fi

  jm_CHECK_DECLARATIONS($headers, free lseek malloc \
                        memchr nanosleep realloc stpcpy strstr strtoul strtoull)
])

dnl FIXME: when autoconf has support for it.
dnl This is a little helper so we can require these header checks.
AC_DEFUN(_jm_DECL_HEADERS,
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(memory.h string.h strings.h stdlib.h unistd.h sys/time.h)
])
