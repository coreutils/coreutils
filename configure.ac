dnl Process this file with autoconf to produce a configure script. -*-m4-*-

dnl Copyright (C) 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2005,
dnl 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.

dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software Foundation,
dnl Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

dnl Written by Jim Meyering.

AC_INIT([GNU coreutils],[5.92],[bug-coreutils@gnu.org])
AC_CONFIG_SRCDIR(src/ls.c)

AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_HEADERS([config.h:config.hin])

AB_INIT()
AM_INIT_AUTOMAKE([1.8.3 gnits dist-bzip2])

gl_DEFAULT_POSIX2_VERSION
gl_USE_SYSTEM_EXTENSIONS
gl_PERL
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GCC_TRADITIONAL
AC_PROG_RANLIB
AC_PROG_LN_S


AC_CHECK_FUNCS(uname,
	OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS uname\$(EXEEXT)"
	MAN="$MAN uname.1")
AC_CHECK_FUNCS(chroot,
	OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS chroot\$(EXEEXT)"
	MAN="$MAN chroot.1")
AC_CHECK_FUNCS(gethostid,
	OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS hostid\$(EXEEXT)"
	MAN="$MAN hostid.1")

gl_MACROS

gl_WINSIZE_IN_PTEM

AC_MSG_CHECKING(whether localtime caches TZ)
AC_CACHE_VAL(utils_cv_localtime_cache,
[if test x$ac_cv_func_tzset = xyes; then
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <time.h>
#if STDC_HEADERS
# include <stdlib.h>
#endif
extern char **environ;
unset_TZ ()
{
  char **from, **to;
  for (to = from = environ; (*to = *from); from++)
    if (! (to[0][0] == 'T' && to[0][1] == 'Z' && to[0][2] == '='))
      to++;
}
main()
{
  time_t now = time ((time_t *) 0);
  int hour_GMT0, hour_unset;
  if (putenv ("TZ=GMT0") != 0)
    exit (1);
  hour_GMT0 = localtime (&now)->tm_hour;
  unset_TZ ();
  hour_unset = localtime (&now)->tm_hour;
  if (putenv ("TZ=PST8") != 0)
    exit (1);
  if (localtime (&now)->tm_hour == hour_GMT0)
    exit (1);
  unset_TZ ();
  if (localtime (&now)->tm_hour != hour_unset)
    exit (1);
  exit (0);
}]])],
[utils_cv_localtime_cache=no],
[utils_cv_localtime_cache=yes],
[# If we have tzset, assume the worst when cross-compiling.
utils_cv_localtime_cache=yes])
else
	# If we lack tzset, report that localtime does not cache TZ,
	# since we can't invalidate the cache if we don't have tzset.
	utils_cv_localtime_cache=no
fi])dnl
AC_MSG_RESULT($utils_cv_localtime_cache)
if test $utils_cv_localtime_cache = yes; then
  AC_DEFINE(LOCALTIME_CACHE, 1, [FIXME])
fi

# SCO-ODT-3.0 is reported to need -los to link programs using initgroups
AC_CHECK_FUNCS(initgroups)
if test $ac_cv_func_initgroups = no; then
  AC_CHECK_LIB(os, initgroups)
fi

AC_CHECK_FUNCS(syslog)
if test $ac_cv_func_syslog = no; then
  # syslog is not in the default libraries.  See if it's in some other.
  for lib in bsd socket inet; do
    AC_CHECK_LIB($lib, syslog, [AC_DEFINE(HAVE_SYSLOG, 1, [FIXME])
      LIBS="$LIBS -l$lib"; break])
  done
fi

AC_CACHE_CHECK([for 3-argument setpriority function],
  [utils_cv_func_setpriority],
  [AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[#include <sys/time.h>
	 #include <sys/resource.h>
       ]],
       [[setpriority (0, 0, 0);]])],
    [utils_cv_func_setpriority=yes],
    [utils_cv_func_setpriority=no])])
if test $utils_cv_func_setpriority = no; then
  AC_CHECK_FUNCS([nice])
fi
case $utils_cv_func_setpriority,$ac_cv_func_nice in
*yes*)
  OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS nice\$(EXEEXT)"
  MAN="$MAN nice.1";;
esac

AC_DEFUN([coreutils_DUMMY_1],
[
  AC_REQUIRE([gl_READUTMP])
  if test $ac_cv_header_utmp_h = yes || test $ac_cv_header_utmpx_h = yes; then
    OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS pinky\$(EXEEXT)"
    OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS users\$(EXEEXT)"
    OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS who\$(EXEEXT)"
    MAN="$MAN pinky.1 users.1 who.1"
  fi
])
coreutils_DUMMY_1

AC_MSG_CHECKING(ut_host in struct utmp)
AC_CACHE_VAL(su_cv_func_ut_host_in_utmp,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <utmp.h>]], [[struct utmp ut; ut.ut_host;]])],
  [su_cv_func_ut_host_in_utmp=yes],
  [su_cv_func_ut_host_in_utmp=no])])
AC_MSG_RESULT($su_cv_func_ut_host_in_utmp)
if test $su_cv_func_ut_host_in_utmp = yes; then
  have_ut_host=1
  AC_DEFINE(HAVE_UT_HOST, 1, [FIXME])
fi

if test -z "$have_ut_host"; then
  AC_MSG_CHECKING(ut_host in struct utmpx)
  AC_CACHE_VAL(su_cv_func_ut_host_in_utmpx,
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <utmpx.h>]], [[struct utmpx ut; ut.ut_host;]])],
    [su_cv_func_ut_host_in_utmpx=yes],
    [su_cv_func_ut_host_in_utmpx=no])])
  AC_MSG_RESULT($su_cv_func_ut_host_in_utmpx)
  if test $su_cv_func_ut_host_in_utmpx = yes; then
    AC_DEFINE(HAVE_UTMPX_H, 1, [FIXME])
    AC_DEFINE(HAVE_UT_HOST, 1, [FIXME])
  fi
fi

GNULIB_BOOT_TIME(
  [OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS uptime\$(EXEEXT)"
  MAN="$MAN uptime.1"])

AC_SYS_POSIX_TERMIOS()
gl_HEADER_TIOCGWINSZ_NEEDS_SYS_IOCTL

if test $ac_cv_sys_posix_termios = yes; then
  OPTIONAL_BIN_PROGS="$OPTIONAL_BIN_PROGS stty\$(EXEEXT)" MAN="$MAN stty.1"

  AC_MSG_CHECKING(whether termios.h needs _XOPEN_SOURCE)
  AC_CACHE_VAL(su_cv_sys_termios_needs_xopen_source,
  [AC_EGREP_CPP(yes, [#include <termios.h>
#ifdef IUCLC
yes
#endif], su_cv_sys_termios_needs_xopen_source=no,
   AC_EGREP_CPP(yes, [#define _XOPEN_SOURCE
#include <termios.h>
#ifdef IUCLC
yes
#endif], su_cv_sys_termios_needs_xopen_source=yes,
   su_cv_sys_termios_needs_xopen_source=no))])
  AC_MSG_RESULT($su_cv_sys_termios_needs_xopen_source)
  test $su_cv_sys_termios_needs_xopen_source = yes &&
    AC_DEFINE(TERMIOS_NEEDS_XOPEN_SOURCE, 1, [FIXME])

  AC_MSG_CHECKING(c_line in struct termios)
  AC_CACHE_VAL(su_cv_sys_c_line_in_termios,
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#if TERMIOS_NEEDS_XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <sys/types.h>
#include <termios.h>]], [[struct termios t; t.c_line;]])],
    [su_cv_sys_c_line_in_termios=yes],
    [su_cv_sys_c_line_in_termios=no])])
  AC_MSG_RESULT($su_cv_sys_c_line_in_termios)
  test $su_cv_sys_c_line_in_termios = yes \
    && AC_DEFINE(HAVE_C_LINE, 1, [FIXME])
fi

# FIXME: note that this macro appears above, too.
# I'm leaving it here for now.  This whole thing needs to be modernized...
gl_WINSIZE_IN_PTEM

gl_HEADER_TIOCGWINSZ_IN_TERMIOS_H

if test $jm_cv_sys_tiocgwinsz_needs_termios_h = no && \
   test $jm_cv_sys_tiocgwinsz_needs_sys_ioctl_h = no; then
  AC_MSG_CHECKING(TIOCGWINSZ in sys/pty.h)
  AC_CACHE_VAL(su_cv_sys_tiocgwinsz_in_sys_pty_h,
  [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#ifdef WINSIZE_IN_PTEM
# include <sys/stream.h>
# include <sys/ptem.h>
#endif
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/pty.h>]], [[int x = TIOCGWINSZ;]])],
    [su_cv_sys_tiocgwinsz_in_sys_pty_h=yes],
    [su_cv_sys_tiocgwinsz_in_sys_pty_h=no])])
  AC_MSG_RESULT($su_cv_sys_tiocgwinsz_in_sys_pty_h)

  test $su_cv_sys_tiocgwinsz_in_sys_pty_h = yes \
      && AC_DEFINE(GWINSZ_IN_SYS_PTY, 1,
                   [Define if your system defines TIOCGWINSZ in sys/pty.h.])
fi

# For src/kill.c.
AC_CHECK_DECLS([strsignal, sys_siglist, _sys_siglist, __sys_siglist], , ,
  [AC_INCLUDES_DEFAULT
#include <signal.h>])

# For src/kill.c and src/printf.c.
AC_CHECK_DECLS([strtoimax, strtoumax])

cu_LIB_CHECK

AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION(0.13.1)

AC_CONFIG_FILES(
  Makefile
  doc/Makefile
  lib/Makefile
  man/Makefile
  po/Makefile.in
  src/Makefile
  tests/Makefile
  tests/basename/Makefile
  tests/chgrp/Makefile
  tests/chmod/Makefile
  tests/chown/Makefile
  tests/cp/Makefile
  tests/cut/Makefile
  tests/dd/Makefile
  tests/dircolors/Makefile
  tests/du/Makefile
  tests/expr/Makefile
  tests/factor/Makefile
  tests/fmt/Makefile
  tests/head/Makefile
  tests/install/Makefile
  tests/join/Makefile
  tests/ln/Makefile
  tests/ls-2/Makefile
  tests/ls/Makefile
  tests/md5sum/Makefile
  tests/misc/Makefile
  tests/mkdir/Makefile
  tests/mv/Makefile
  tests/od/Makefile
  tests/pr/Makefile
  tests/readlink/Makefile
  tests/rm/Makefile
  tests/rmdir/Makefile
  tests/seq/Makefile
  tests/sha1sum/Makefile
  tests/shred/Makefile
  tests/sort/Makefile
  tests/stty/Makefile
  tests/sum/Makefile
  tests/tac/Makefile
  tests/tail-2/Makefile
  tests/tail/Makefile
  tests/tee/Makefile
  tests/test/Makefile
  tests/touch/Makefile
  tests/tr/Makefile
  tests/tsort/Makefile
  tests/unexpand/Makefile
  tests/uniq/Makefile
  tests/wc/Makefile
  )
AC_OUTPUT
