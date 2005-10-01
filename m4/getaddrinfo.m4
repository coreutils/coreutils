# getaddrinfo.m4 serial 6
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
  AC_CHECK_HEADERS_ONCE(sys/socket.h netdb.h sys/types.h netinet/in.h)
  AC_CHECK_DECLS([getaddrinfo, freeaddrinfo, gai_strerror],,,[
  /* sys/types.h is not needed according to POSIX, but the
     sys/socket.h in i386-unknown-freebsd4.10 and
     powerpc-apple-darwin5.5 required it. */
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
])
  AC_CHECK_TYPES([struct addrinfo],,,[
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
])
])
