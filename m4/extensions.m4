# gl_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
AC_DEFUN([gl_USE_SYSTEM_EXTENSIONS], [
  AC_REQUIRE([AC_GNU_SOURCE])
  AH_VERBATIM([__EXTENSIONS__],
[/* Enable extensions on Solaris.  */
#ifndef __EXTENSIONS__
# undef __EXTENSIONS__
#endif])
  AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
  AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
  AC_DEFINE([__EXTENSIONS__])
])
