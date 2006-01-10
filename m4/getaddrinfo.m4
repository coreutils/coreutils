# getaddrinfo.m4 serial 7
dnl Copyright (C) 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETADDRINFO],
[
  AC_SEARCH_LIBS(getaddrinfo, [nsl socket])
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])
  AC_SEARCH_LIBS(getservbyname, [inet nsl socket xnet])
  AC_REPLACE_FUNCS(getaddrinfo gai_strerror)
  gl_PREREQ_GETADDRINFO
])

# Prerequisites of lib/getaddrinfo.h and lib/getaddrinfo.c.
AC_DEFUN([gl_PREREQ_GETADDRINFO], [
  AC_REQUIRE([gl_C_RESTRICT])
  AC_REQUIRE([gl_SOCKET_FAMILIES])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_CHECK_HEADERS_ONCE(netinet/in.h)
  AC_CHECK_DECLS([getaddrinfo, freeaddrinfo, gai_strerror],,,[
  /* sys/types.h is not needed according to POSIX, but the
     sys/socket.h in i386-unknown-freebsd4.10 and
     powerpc-apple-darwin5.5 required it. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
])
  AC_CHECK_TYPES([struct addrinfo],,,[
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
])
])
