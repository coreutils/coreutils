#serial 3

# Copyright (C) 2005 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([gl_UNLINKDIR],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CHECK_HEADERS_ONCE(priv.h)

  AC_LIBSOURCES([unlinkdir.c, unlinkdir.h])
  AC_LIBOBJ([unlinkdir])

  # The Hurd, the Linux kernel, the FreeBSD kernel version 2.2 and later,
  # and Cygwin never let anyone (even root) unlink directories.
  # If anyone knows of another system for which unlink can never
  # remove a directory, please report it to <bug-coreutils@gnu.org>.
  # Unfortunately this is difficult to test for, since it requires root access
  # and might create garbage in the file system,
  # so the code below simply relies on the kernel name and version number.
  case $host in
  *-*-gnu[0-9]* | \
  *-*-linux-* | *-*-linux | \
  *-*-freebsd2.2* | *-*-freebsd[3-9]* | *-*-freebsd[1-9][0-9]* | \
  *-cygwin)
    AC_DEFINE([UNLINK_CANNOT_UNLINK_DIR], 1,
      [Define to 1 if unlink (dir) cannot possibly succeed.]);;
  esac
])
