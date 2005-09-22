# sockpfaf.m4 serial 2
dnl Copyright (C) 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Test for some common socket protocol families (PF_INET, PF_INET6, ...)
dnl and some common address families (AF_INET, AF_INET6, ...).
dnl This test assumes that a system supports an address family if and only if
dnl it supports the corresponding protocol family.

dnl From Bruno Haible.

AC_DEFUN([gl_SOCKET_FAMILIES],
[
  AC_MSG_CHECKING(for IPv4 sockets)
  AC_CACHE_VAL(gl_cv_socket_ipv4,
    [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
[int x = AF_INET; struct in_addr y; struct sockaddr_in z;],
       gl_cv_socket_ipv4=yes, gl_cv_socket_ipv4=no)])
  AC_MSG_RESULT($gl_cv_socket_ipv4)
  if test $gl_cv_socket_ipv4 = yes; then
    AC_DEFINE(HAVE_IPV4, 1, [Define to 1 if <sys/socket.h> defines AF_INET.])
  fi

  AC_MSG_CHECKING(for IPv6 sockets)
  AC_CACHE_VAL(gl_cv_socket_ipv6,
    [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>],
[int x = AF_INET6; struct in6_addr y; struct sockaddr_in6 z;],
       gl_cv_socket_ipv6=yes, gl_cv_socket_ipv6=no)])
  AC_MSG_RESULT($gl_cv_socket_ipv6)
  if test $gl_cv_socket_ipv6 = yes; then
    AC_DEFINE(HAVE_IPV6, 1, [Define to 1 if <sys/socket.h> defines AF_INET6.])
  fi
])
