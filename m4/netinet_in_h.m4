# netinet_in_h.m4 serial 1
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Simon Josefsson

AC_DEFUN([gl_HEADER_NETINET_IN],
[
  AC_CHECK_HEADERS_ONCE([netinet/in.h])
  if test $ac_cv_header_netinet_in_h = yes; then
    NETINET_IN_H=''
  else
    NETINET_IN_H='netinet/in.h'
  fi
  AC_SUBST(NETINET_IN_H)
])
