# canon-host.m4 serial 6
dnl Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CANON_HOST],
[
  AC_LIBSOURCES([canon-host.c])
  AC_LIBOBJ([canon-host])

  dnl Prerequisites of lib/canon-host.c.
  AC_CHECK_HEADERS_ONCE(unistd.h)
  AC_CHECK_HEADERS(netdb.h sys/socket.h netinet/in.h arpa/inet.h)

  dnl Add any libraries as early as possible.
  dnl In particular, inet_ntoa needs -lnsl at least on Solaris 2.5.1,
  dnl so we have to add -lnsl to LIBS before checking for that function.
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])

  dnl These come from -lnsl on Solaris 2.5.1.
  AC_CHECK_FUNCS(getaddrinfo gethostbyname gethostbyaddr inet_ntoa)
])
