#serial 6

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(jm_CHECK_DECLS,
[
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
#if STDC_HEADERS
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
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
  fi

  jm_CHECK_DECLARATIONS($headers, free lseek malloc \
                        memchr realloc stpcpy strstr strtoul strtoull)
])
