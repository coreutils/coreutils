# acl.m4 - check for access control list (ACL) primitives

# Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([AC_FUNC_ACL],
[
  AC_LIBSOURCES([acl.c, acl.h])
  AC_LIBOBJ([acl])

  dnl Prerequisites of lib/acl.c.
  AC_CHECK_HEADERS(sys/acl.h)
  AC_CHECK_FUNCS(acl)
])
