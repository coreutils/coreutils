# Enable extensions on systems that normally disable them.

# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# gl_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
AC_DEFUN([gl_USE_SYSTEM_EXTENSIONS], [
  AC_BEFORE([$0], [AC_COMPILE_IFELSE])
  AC_BEFORE([$0], [AC_RUN_IFELSE])

  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([AC_AIX])
  AC_REQUIRE([AC_MINIX])

  AH_VERBATIM([__EXTENSIONS__],
[/* Enable extensions on Solaris.  */
#ifndef __EXTENSIONS__
# undef __EXTENSIONS__
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
# undef _POSIX_PTHREAD_SEMANTICS
#endif])
  AC_DEFINE([__EXTENSIONS__])
  AC_DEFINE([_POSIX_PTHREAD_SEMANTICS])
])
