#serial 3

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(jm_CHECK_DECLS,
[
  headers='
#include <stdio.h>
#ifdef HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
'
  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_CHECK_FUNCS(DECL_FREE DECL_LSEEK DECL_MALLOC DECL_MEMCHR DECL_REALLOC \
		   DECL_STPCPY DECL_STRSTR)
  fi
  jm_CHECK_DECLARATIONS($headers, free lseek malloc \
                        memchr realloc stpcpy strstr)

  # Check for a declaration of localtime_r.
  jm_CHECK_DECL_LOCALTIME_R
])

dnl localtime_r is a special case...
dnl Code that uses the result of this test must use the same cpp
dnl directives as are used below.  Also include the following declaration
dnl after the inclusion of time.h.
dnl
dnl #if ! defined HAVE_DECL_LOCALTIME_R
dnl extern struct tm* localtime_r ();
dnl #endif
AC_DEFUN(jm_CHECK_DECL_LOCALTIME_R,
[
  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_CHECK_FUNCS(DECL_LOCALTIME_R)
  fi

  headers='
/* Some systems need this in order to declare localtime_r properly.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <time.h>
'
  jm_CHECK_DECLARATIONS($headers, localtime_r)
])
