# physmem.m4 serial 5
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Check for the external symbol, _system_configuration,
# a struct with member `physmem'.
AC_DEFUN([gl_SYS__SYSTEM_CONFIGURATION],
  [AC_CACHE_CHECK(for external symbol _system_configuration,
		  gl_cv_var__system_configuration,
    [AC_LINK_IFELSE([AC_LANG_PROGRAM(
		      [[#include <sys/systemcfg.h>
		      ]],
		      [double x = _system_configuration.physmem;])],
      [gl_cv_var__system_configuration=yes],
      [gl_cv_var__system_configuration=no])])

    if test $gl_cv_var__system_configuration = yes; then
      AC_DEFINE(HAVE__SYSTEM_CONFIGURATION, 1,
		[Define to 1 if you have the external variable,
		_system_configuration with a member named physmem.])
    fi
  ]
)

AC_DEFUN([gl_PHYSMEM],
[
  AC_LIBSOURCES([physmem.c, physmem.h])
  AC_LIBOBJ([physmem])

  # Prerequisites of lib/physmem.c.
  AC_CHECK_HEADERS([sys/pstat.h sys/sysmp.h sys/sysinfo.h \
    machine/hal_sysinfo.h sys/table.h sys/param.h sys/sysctl.h \
    sys/systemcfg.h],,, [AC_INCLUDES_DEFAULT])

  AC_CHECK_FUNCS(pstat_getstatic pstat_getdynamic sysmp getsysinfo sysctl table)
  AC_REQUIRE([gl_SYS__SYSTEM_CONFIGURATION])
])
