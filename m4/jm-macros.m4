#serial 109   -*- autoconf -*-

dnl Misc type-related macros for coreutils.

# Copyright (C) 1998, 2000-2009 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Written by Jim Meyering.

AC_DEFUN([coreutils_MACROS],
[
  AM_MISSING_PROG(HELP2MAN, help2man)
  AC_SUBST([MAN])

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  gl_CHECK_ALL_TYPES

  AC_REQUIRE([gl_CHECK_DECLS])

  AC_REQUIRE([gl_PREREQ])

  AC_REQUIRE([AC_FUNC_FSEEKO])

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE([ARGMATCH_DIE], [usage (1)],
            [Define to the function xargmatch calls on failures.])
  AC_DEFINE([ARGMATCH_DIE_DECL], [void usage (int _e)],
            [Define to the declaration of the xargmatch failure function.])

  # used by ls
  AC_REQUIRE([gl_CLOCK_TIME])
  # used by shred
  AC_CHECK_FUNCS_ONCE([directio])

  # Used by install.c.
  coreutils_saved_libs=$LIBS
    LIBS="$LIBS $LIB_SELINUX"
    AC_CHECK_FUNCS([matchpathcon_init_prefix], [],
    [
      case "$ac_cv_search_setfilecon:$ac_cv_header_selinux_selinux_h" in
        no:*) # SELinux disabled
          ;;
        *:no) # SELinux disabled
          ;;
        *)
        AC_MSG_WARN([SELinux enabled, but matchpathcon_init_prefix not found])
        AC_MSG_WARN([The install utility may run slowly])
      esac
    ])
  LIBS=$coreutils_saved_libs

  # Used by sort.c.
  AC_CHECK_FUNCS_ONCE([nl_langinfo])

  # Used by tail.c.
  AC_CHECK_FUNCS([inotify_init],
    [AC_DEFINE([HAVE_INOTIFY], [1],
     [Define to 1 if you have usable inotify support.])])

  AC_CHECK_FUNCS_ONCE( \
    endgrent \
    endpwent \
    fchown \
    fchmod \
    ftruncate \
    iswspace \
    mkfifo \
    mbrlen \
    setgroups \
    sethostname \
    siginterrupt \
    sync \
    sysctl \
    sysinfo \
    tcgetpgrp \
  )

  # for cp.c
  AC_CHECK_FUNCS_ONCE([utimensat])

  dnl This can't use AC_REQUIRE; I'm not quite sure why.
  cu_PREREQ_STAT_PROG

  # for dd.c and shred.c
  coreutils_saved_libs=$LIBS
    AC_SEARCH_LIBS([fdatasync], [rt posix4],
                   [test "$ac_cv_search_fdatasync" = "none required" ||
                    LIB_FDATASYNC=$ac_cv_search_fdatasync])
    AC_SUBST([LIB_FDATASYNC])
    AC_CHECK_FUNCS([fdatasync])
  LIBS=$coreutils_saved_libs

  # Check whether libcap is usable -- for ls --color support
  LIB_CAP=
  AC_ARG_ENABLE([libcap],
    AC_HELP_STRING([--disable-libcap], [disable libcap support]))
  if test "X$enable_libcap" != "Xno"; then
    AC_CHECK_LIB([cap], [cap_get_file],
      [AC_CHECK_HEADER([sys/capability.h],
        [LIB_CAP=-lcap
         AC_DEFINE([HAVE_CAP], [1], [libcap usability])]
      )])
    if test "X$LIB_CAP" = "X"; then
      if test "X$enable_libcap" = "Xyes"; then
        AC_MSG_ERROR([libcap library was not found or not usable])
      else
        AC_MSG_WARN([libcap library was not found or not usable, support for libcap will not be built])
      fi
    fi
  else
    AC_MSG_WARN([libcap support disabled by user])
  fi
  AC_SUBST([LIB_CAP])

  # See if linking `seq' requires -lm.
  # It does on nearly every system.  The single exception (so far) is
  # BeOS which has all the math functions in the normal runtime library
  # and doesn't have a separate math library.

  AC_SUBST([SEQ_LIBM])
  ac_seq_body='
     static double x, y;
     x = floor (x);
     x = rint (x);
     x = modf (x, &y);'
  AC_TRY_LINK([#include <math.h>], [$ac_seq_body], ,
    [ac_seq_save_LIBS="$LIBS"
     LIBS="$LIBS -lm"
     AC_TRY_LINK([#include <math.h>], [$ac_seq_body], [SEQ_LIBM=-lm])
     LIBS="$ac_seq_save_LIBS"
    ])

  AC_REQUIRE([AM_LANGINFO_CODESET])
])

AC_DEFUN([gl_CHECK_ALL_HEADERS],
[
  AC_CHECK_HEADERS_ONCE( \
    hurd.h \
    paths.h \
    priv.h \
    stropts.h \
    sys/ioctl.h \
    sys/param.h \
    sys/resource.h \
    sys/systeminfo.h \
    sys/time.h \
    sys/wait.h \
    syslog.h \
    termios.h \
  )
  AC_CHECK_HEADERS([sys/sysctl.h], [], [],
    [AC_INCLUDES_DEFAULT
     [#if HAVE_SYS_PARAM_H
       #include <sys/param.h>
      #endif]])
])

# This macro must be invoked before any tests that run the compiler.
AC_DEFUN([gl_CHECK_ALL_TYPES],
[
  dnl This test must come as early as possible after the compiler configuration
  dnl tests, because the choice of the file model can (in principle) affect
  dnl whether functions and headers are available, whether they work, etc.
  AC_REQUIRE([AC_SYS_LARGEFILE])

  dnl This test must precede tests of compiler characteristics like
  dnl that for the inline keyword, since it may change the degree to
  dnl which the compiler supports such features.
  AC_REQUIRE([AM_C_PROTOTYPES])

  dnl Checks for typedefs, structures, and compiler characteristics.
  AC_REQUIRE([gl_BIGENDIAN])
  AC_REQUIRE([AC_C_VOLATILE])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])

  AC_REQUIRE([gl_CHECK_ALL_HEADERS])
  AC_CHECK_MEMBERS(
    [struct stat.st_author],,,
    [$ac_includes_default
#include <sys/stat.h>
  ])
  AC_REQUIRE([AC_STRUCT_ST_BLOCKS])

  AC_REQUIRE([AC_TYPE_GETGROUPS])
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_REQUIRE([AC_TYPE_PID_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_CHECK_TYPE([ino_t], [unsigned long int])

  dnl This relies on the fact that Autoconf's implementation of
  dnl AC_CHECK_TYPE checks includes unistd.h.
  AC_CHECK_TYPE([major_t], [unsigned int])
  AC_CHECK_TYPE([minor_t], [unsigned int])

  AC_REQUIRE([AC_HEADER_MAJOR])
])
