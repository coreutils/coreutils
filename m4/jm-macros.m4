#serial 116   -*- autoconf -*-

dnl Misc type-related macros for coreutils.

# Copyright (C) 1998-2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Written by Jim Meyering.

AC_DEFUN([coreutils_MACROS],
[
  AM_MISSING_PROG(HELP2MAN, help2man)
  AC_SUBST([MAN])

  gl_CHECK_ALL_TYPES

  AC_REQUIRE([gl_CHECK_DECLS])

  AC_REQUIRE([gl_PREREQ])

  AC_REQUIRE([AC_FUNC_FSEEKO])

  # By default, argmatch should fail calling usage (EXIT_FAILURE).
  AC_DEFINE([ARGMATCH_DIE], [usage (EXIT_FAILURE)],
            [Define to the function xargmatch calls on failures.])
  AC_DEFINE([ARGMATCH_DIE_DECL], [void usage (int _e)],
            [Define to the declaration of the xargmatch failure function.])

  # Ensure VLAs are not used.
  # Note -Wvla is implicitly added by gl_MANYWARN_ALL_GCC
  AC_DEFINE([GNULIB_NO_VLA], [1], [Define to 1 to disable use of VLAs])

  # used by shred
  AC_CHECK_FUNCS_ONCE([directio])

  coreutils_saved_libs=$LIBS
    LIBS="$LIBS $LIB_SELINUX"
    # Used by selinux.c.
    AC_CHECK_FUNCS([mode_to_security_class], [], [])
  LIBS=$coreutils_saved_libs

  # Used by sort.c.
  AC_CHECK_FUNCS_ONCE([nl_langinfo])
  # Used by timeout.c
  AC_CHECK_FUNCS_ONCE([setitimer setrlimit prctl])

  # Used by tail.c.
  AC_CHECK_FUNCS([inotify_init],
    [AC_DEFINE([HAVE_INOTIFY], [1],
     [Define to 1 if you have usable inotify support.])])

  AC_CHECK_FUNCS_ONCE([
    endgrent
    endpwent
    fallocate
    fchown
    fchmod
    setgroups
    sethostname
    siginterrupt
    sync
    sysinfo
    tcgetpgrp
  ])

  # Android API level 30.
  gl_CHECK_FUNCS_ANDROID([syncfs], [[#include <unistd.h>]])

  # These checks are for Interix, to avoid its getgr* functions, in favor
  # of these replacements.  The replacement functions are much more efficient
  # because they do not query the domain controller for user information
  # when it is not needed.
  AC_CHECK_FUNCS_ONCE([
    getgrgid_nomembers
    getgrnam_nomembers
    getgrent_nomembers
  ])

  dnl This can't use AC_REQUIRE; I'm not quite sure why.
  cu_PREREQ_STAT_PROG

  # Check whether libcap is usable -- for ls --color support
  LIB_CAP=
  AC_ARG_ENABLE([libcap],
    AS_HELP_STRING([--disable-libcap], [disable libcap support]))
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
        AC_MSG_WARN([libcap library was not found or not usable.])
        AC_MSG_WARN([AC_PACKAGE_NAME will be built without capability support.])
      fi
    fi
  else
    AC_MSG_WARN([libcap support disabled by user])
  fi
  AC_SUBST([LIB_CAP])

  # See if linking 'seq' requires -lm.
  # It does on nearly every system.  The single exception (so far) is
  # BeOS which has all the math functions in the normal runtime library
  # and doesn't have a separate math library.

  AC_SUBST([SEQ_LIBM])
  jm_break=:
  for jm_seqlibs in '' '-lm'; do
    jm_seq_save_LIBS=$LIBS
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
         [[#include <math.h>
         ]],
         [[static double x, y;
           x = floor (x);
           x = rint (x);
           x = modf (x, &y);]])],
      [SEQ_LIBM=$jm_seqlibs
       jm_break=break])
    LIBS=$jm_seq_save_LIBS
    $jm_break
  done

  # See is fpsetprec() required to use extended double precision
  # This is needed on 32 bit FreeBSD to give accurate conversion of:
  # `numfmt 9223372036854775808`
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
       [[#include <ieeefp.h>
       ]],
       [[#ifdef __i386__
          fpsetprec (FP_PE);
         #else
         # error not required on 64 bit
         #endif
       ]])],
    [ac_have_fpsetprec=yes],
    [ac_have_fpsetprec=no])
  if test "$ac_have_fpsetprec" = "yes" ; then
    AC_DEFINE([HAVE_FPSETPREC], 1, [whether fpsetprec is present and required])
  fi

  AC_REQUIRE([AM_LANGINFO_CODESET])

  # Accept configure options: --with-tty-group[=GROUP], --without-tty-group
  # You can determine the group of a TTY via 'stat --format %G /dev/tty'
  # Omitting this option is equivalent to using --without-tty-group.
  AC_ARG_WITH([tty-group],
    AS_HELP_STRING([--with-tty-group[[[=NAME]]]],
      [group used by system for TTYs, "tty" when not specified]
      [ (default: do not rely on any group used for TTYs)]),
    [tty_group_name=$withval],
    [tty_group_name=no])

  if test "x$tty_group_name" != xno; then
    if test "x$tty_group_name" = xyes; then
      tty_group_name=tty
    fi
    AC_MSG_NOTICE([TTY group used by system set to "$tty_group_name"])
    AC_DEFINE_UNQUOTED([TTY_GROUP_NAME], ["$tty_group_name"],
      [group used by system for TTYs])
  fi
])

AC_DEFUN([gl_CHECK_ALL_HEADERS],
[
  AC_CHECK_HEADERS_ONCE([
    hurd.h
    linux/falloc.h
    linux/fs.h
    paths.h
    priv.h
    stropts.h
    sys/mtio.h
    sys/param.h
    sys/systeminfo.h
    syslog.h
  ])
  AC_CHECK_HEADERS([sys/sysctl.h], [], [],
    [AC_INCLUDES_DEFAULT
     [#if HAVE_SYS_PARAM_H
       #include <sys/param.h>
      #endif]])
])

# This macro must be invoked before any tests that run the compiler.
AC_DEFUN([gl_CHECK_ALL_TYPES],
[
  dnl Checks for typedefs, structures, and compiler characteristics.
  AC_REQUIRE([gl_BIGENDIAN])
  AC_REQUIRE([AC_C_VOLATILE])
  AC_REQUIRE([AC_C_INLINE])

  AC_REQUIRE([gl_CHECK_ALL_HEADERS])
  AC_CHECK_MEMBERS(
    [struct stat.st_author],,,
    [$ac_includes_default
#include <sys/stat.h>
  ])
  AC_REQUIRE([AC_STRUCT_ST_BLOCKS])

  AC_REQUIRE([AC_TYPE_GETGROUPS])

  dnl FIXME is this section still needed?
  dnl These types are universally available now.
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_REQUIRE([AC_TYPE_PID_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_CHECK_TYPE([ino_t], [],
    [AC_DEFINE([ino_t], [unsigned long int],
       [Type of file serial numbers, also known as inode numbers.])])

  dnl This relies on the fact that Autoconf's implementation of
  dnl AC_CHECK_TYPE checks includes unistd.h.
  AC_CHECK_TYPE([major_t], [],
    [AC_DEFINE([major_t], [unsigned int], [Type of major device numbers.])])
  AC_CHECK_TYPE([minor_t], [],
    [AC_DEFINE([minor_t], [unsigned int], [Type of minor device numbers.])])

  AC_REQUIRE([AC_HEADER_MAJOR])
])
