#serial 5

AC_PREREQ(2.13)

AC_DEFUN([jm_SYS_PROC_UPTIME],
[ dnl Require AC_PROG_CC to see if we're cross compiling.
  AC_REQUIRE([AC_PROG_CC])
  AC_CACHE_CHECK([for /proc/uptime], jm_cv_have_proc_uptime,
  [jm_cv_have_proc_uptime=no
    test -f /proc/uptime \
      && test "$cross_compiling" = no \
      && cat < /proc/uptime >/dev/null 2>/dev/null \
      && jm_cv_have_proc_uptime=yes])
  if test $jm_cv_have_proc_uptime = yes; then
    AC_DEFINE(HAVE_PROC_UPTIME, 1,
	      [  Define if your system has the /proc/uptime special file.])
  fi
])
