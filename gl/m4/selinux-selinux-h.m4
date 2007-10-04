# serial 1   -*- Autoconf -*-
# Copyright (C) 2006, 2007 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# From Jim Meyering
# Provide <selinux/selinux.h>, if necessary.

AC_DEFUN([gl_HEADERS_SELINUX_SELINUX_H],
[
  # Check for <selinux/selinux.h>,
  AC_CHECK_HEADERS([selinux/selinux.h],
		   [SELINUX_SELINUX_H=],
		   [SELINUX_SELINUX_H=selinux/selinux.h])
  AC_SUBST([SELINUX_SELINUX_H])
])
