# Determine whether this system has infrastructure for obtaining the boot time.

# GNULIB_BOOT_TIME([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
* ----------------------------------------------------------
AC_DEFUN([GNULIB_BOOT_TIME],
[
  AC_CHECK_FUNCS(sysctl)
  AC_CHECK_HEADERS(sys/sysctl.h)
  AC_CACHE_CHECK(
    [whether we can get the system boot time],
    [gnulib_cv_have_boot_time],
    [
      AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#if HAVE_SYSCTL && HAVE_SYS_SYSCTL_H
# include <sys/param.h> /* needed for OpenBSD 3.0 */
# include <sys/sysctl.h>
#endif
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#else
# include <utmp.h>
#endif
],
[[
#if defined BOOT_TIME || (defined CTL_KERN && defined KERN_BOOTTIME)
/* your system *does* have the infrastructure to determine boot time */
#else
please_tell_us_how_to_determine_boot_time_on_your_system
#endif
]])],
       gnulib_cv_have_boot_time=yes,
       gnulib_cv_have_boot_time=no)
    ])
  AS_IF([test $gnulib_cv_have_boot_time = yes], [$1], [$2])
])
