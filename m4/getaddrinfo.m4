# getaddrinfo.m4 serial 8
dnl Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_GETADDRINFO],
[
  AC_MSG_NOTICE([checking how to do getaddrinfo])

  AC_SEARCH_LIBS(getaddrinfo, [nsl socket])
  AC_CHECK_FUNCS(getaddrinfo,, [
    AC_CACHE_CHECK(for getaddrinfo in ws2tcpip.h and -lws2_32,
		   gl_cv_w32_getaddrinfo, [
      gl_cv_w32_getaddrinfo=no
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -lws2_32"
      AC_TRY_LINK([
#ifdef HAVE_WS2TCPIP_H
#define WINVER 0x0501
#include <ws2tcpip.h>
#endif
], [getaddrinfo(0, 0, 0, 0);], gl_cv_w32_getaddrinfo=yes)
      LIBS="$am_save_LIBS"
      if test "$gl_cv_w32_getaddrinfo" = "yes"; then
	LIBS="$LIBS -lws2_32"
      else
	AC_LIBOBJ(getaddrinfo)
      fi
    ])])

  AC_REPLACE_FUNCS(gai_strerror)
  gl_PREREQ_GETADDRINFO
])

# Prerequisites of lib/getaddrinfo.h and lib/getaddrinfo.c.
AC_DEFUN([gl_PREREQ_GETADDRINFO], [
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])
  AC_SEARCH_LIBS(getservbyname, [inet nsl socket xnet])
  AC_REQUIRE([gl_C_RESTRICT])
  AC_REQUIRE([gl_SOCKET_FAMILIES])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_CHECK_HEADERS_ONCE(netinet/in.h netdb.h)
  AC_CHECK_DECLS([getaddrinfo, freeaddrinfo, gai_strerror],,,[
  /* sys/types.h is not needed according to POSIX, but the
     sys/socket.h in i386-unknown-freebsd4.10 and
     powerpc-apple-darwin5.5 required it. */
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#define WINVER 0x0501
#include <ws2tcpip.h>
#endif
])
  AC_CHECK_TYPES([struct addrinfo],,,[
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#define WINVER 0x0501
#include <ws2tcpip.h>
#endif
])
])
