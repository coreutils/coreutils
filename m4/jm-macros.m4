#serial 94   -*- autoconf -*-

dnl Misc type-related macros for coreutils.

# Copyright (C) 1998, 2000, 2001, 2002, 2003, 2004, 2005 Free Software
# Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Written by Jim Meyering.

AC_DEFUN([gl_MACROS],
[
  AC_PREREQ(2.58)

  GNU_PACKAGE="GNU $PACKAGE"
  AC_DEFINE_UNQUOTED(GNU_PACKAGE, "$GNU_PACKAGE",
    [The concatenation of the strings `GNU ', and PACKAGE.])
  AC_SUBST(GNU_PACKAGE)

  AM_MISSING_PROG(HELP2MAN, help2man)
  AC_SUBST(OPTIONAL_BIN_PROGS)
  AC_SUBST(MAN)
  AC_SUBST(DF_PROG)

  dnl This macro actually runs replacement code.  See isc-posix.m4.
  AC_REQUIRE([AC_ISC_POSIX])dnl

  gl_CHECK_ALL_TYPES

  AC_REQUIRE([gl_HOST_OS])
  AC_REQUIRE([gl_ASSERT])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_DIRENT_D_TYPE])
  AC_REQUIRE([gl_CHECK_TYPE_STRUCT_DIRENT_D_INO])
  AC_REQUIRE([gl_CHECK_DECLS])

  AC_REQUIRE([gl_PREREQ])

  AC_REQUIRE([gl_FUNC_DIRFD])
  AC_REQUIRE([AC_FUNC_ACL])
  AC_REQUIRE([gl_FUNC_LCHOWN])
  AC_REQUIRE([gl_FUNC_RMDIR_NOTEMPTY])
  AC_REQUIRE([gl_FUNC_CHOWN])
  AC_REQUIRE([AC_FUNC_LSTAT])
  AC_REQUIRE([AC_FUNC_STRERROR_R])
  AC_REQUIRE([gl_FUNC_GROUP_MEMBER])
  AC_REQUIRE([gl_AFS])
  AC_REQUIRE([gl_AC_FUNC_LINK_FOLLOWS_SYMLINK])
  AC_REQUIRE([gl_FUNC_FPENDING])

  # This is for od and stat, and any other program that
  # uses the PRI.MAX macros from inttypes.h.
  AC_REQUIRE([gt_INTTYPES_PRI])
  AC_REQUIRE([gl_ULLONG_MAX])

  AC_REQUIRE([gl_FUNC_GETGROUPS])

  AC_REQUIRE([AC_FUNC_FSEEKO])
  AC_REQUIRE([AC_FUNC_ALLOCA])

  AC_CONFIG_LIBOBJ_DIR([lib])
  AC_FUNC_GETLOADAVG([lib])

  AC_REQUIRE([gl_SYS_PROC_UPTIME])
  AC_REQUIRE([gl_FUNC_FTRUNCATE])

  # raise is used by at least sort and ls.
  AC_REPLACE_FUNCS(raise)

  # By default, argmatch should fail calling usage (1).
  AC_DEFINE(ARGMATCH_DIE, [usage (1)],
	    [Define to the function xargmatch calls on failures.])
  AC_DEFINE(ARGMATCH_DIE_DECL, [extern void usage ()],
	    [Define to the declaration of the xargmatch failure function.])

  dnl Used to define SETVBUF in sys2.h.
  dnl This evokes the following warning from autoconf:
  dnl ...: warning: AC_TRY_RUN called without default to allow cross compiling
  AC_FUNC_SETVBUF_REVERSED

  # used by sleep and shred
  AC_REQUIRE([gl_CLOCK_TIME])
  AC_CHECK_FUNCS_ONCE(gettimeofday)
  AC_FUNC_GETTIMEOFDAY_CLOBBER
  # used by shred
  AC_CHECK_FUNCS_ONCE(directio)

  AC_REQUIRE([AC_FUNC_CLOSEDIR_VOID])

  AC_CHECK_FUNCS_ONCE( \
    endgrent \
    endpwent \
    fchmod \
    fchown \
    ftruncate \
    hasmntopt \
    isascii \
    iswspace \
    lchown \
    listmntent \
    mempcpy \
    mkfifo \
    mbrlen \
    realpath \
    sethostname \
    siginterrupt \
    sysctl \
    sysinfo \
    tcgetpgrp \
    wcrtomb \
    tzset \
  )

  AC_FUNC_STRTOD

  AC_REQUIRE([cu_PREREQ_STAT_PROG])

  # for dd.c and shred.c
  coreutils_saved_libs=$LIBS
    AC_SEARCH_LIBS([fdatasync], [rt posix4],
		   [test "$ac_cv_search_fdatasync" = "none required" ||
		    LIB_FDATASYNC=$ac_cv_search_fdatasync])
    AC_SUBST([LIB_FDATASYNC])
    AC_CHECK_FUNCS(fdatasync)
  LIBS=$coreutils_saved_libs

  # See if linking `seq' requires -lm.
  # It does on nearly every system.  The single exception (so far) is
  # BeOS which has all the math functions in the normal runtime library
  # and doesn't have a separate math library.

  AC_SUBST(SEQ_LIBM)
  ac_seq_body='
     static double x, y;
     x = floor (x);
     x = rint (x);
     x = modf (x, &y);'
  AC_TRY_LINK([#include <math.h>], $ac_seq_body, ,
    [ac_seq_save_LIBS="$LIBS"
     LIBS="$LIBS -lm"
     AC_TRY_LINK([#include <math.h>], $ac_seq_body, SEQ_LIBM=-lm)
     LIBS="$ac_seq_save_LIBS"
    ])

  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_REQUIRE([gl_GLIBC21])
  AM_ICONV
  gl_FUNC_UNLINK_BUSY_TEXT

  # These tests are for df.
  AC_REQUIRE([gl_FSUSAGE])
  AC_REQUIRE([gl_MOUNTLIST])
  if test $gl_cv_list_mounted_fs = yes && test $gl_cv_fs_space = yes; then
    DF_PROG='df$(EXEEXT)'
  fi
  AC_REQUIRE([gl_AC_DOS])
  AC_REQUIRE([AC_FUNC_CANONICALIZE_FILE_NAME])

  # If any of these functions don't exist (e.g. DJGPP 2.03),
  # use the corresponding stub.
  AC_CHECK_FUNC([fchdir], , [AC_LIBOBJ(fchdir-stub)])

  AC_REQUIRE([gl_FUNC_FREE])
  AC_REQUIRE([gl_FUNC_CHDIR_LONG])
  AC_REQUIRE([gl_FUNC_XFTS])
  AC_REQUIRE([gl_ROOT_DEV_INO])
  AC_REQUIRE([gl_VERSION_ETC])
  AC_REQUIRE([gl_DIACRIT])
  AC_REQUIRE([gl_SOCKLEN_T])
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
    sys/statvfs.h \
    sys/systeminfo.h \
    sys/time.h \
    sys/vfs.h \
    sys/wait.h \
    syslog.h \
    termios.h \
  )
  AC_CHECK_HEADERS(sys/mount.h sys/sysctl.h, [], [],
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
  AC_REQUIRE([AC_C_BIGENDIAN])
  AC_REQUIRE([AC_C_CONST])
  AC_REQUIRE([AC_C_VOLATILE])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_C_LONG_DOUBLE])

  AC_REQUIRE([gl_CHECK_ALL_HEADERS])
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_MEMBERS(
    [struct stat.st_author,
     struct stat.st_blksize],,,
    [$ac_includes_default
#include <sys/stat.h>
  ])
  AC_REQUIRE([AC_STRUCT_ST_BLOCKS])

  AC_REQUIRE([AC_STRUCT_ST_DM_MODE])

  AC_REQUIRE([AC_TYPE_GETGROUPS])
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  AC_REQUIRE([AC_TYPE_MODE_T])
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_REQUIRE([AC_TYPE_PID_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_CHECK_TYPE(ino_t, unsigned long int)

  gt_TYPE_SSIZE_T

  dnl This relies on the fact that autoconf 2.14a's implementation of
  dnl AC_CHECK_TYPE checks includes unistd.h.
  AC_CHECK_TYPE(major_t, unsigned int)
  AC_CHECK_TYPE(minor_t, unsigned int)

  AC_REQUIRE([gl_AC_TYPE_UINT32_T])
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
  AC_REQUIRE([gl_AC_TYPE_UINTPTR_T])
  AC_REQUIRE([gl_AC_TYPE_UNSIGNED_LONG_LONG])

  AC_REQUIRE([AC_HEADER_MAJOR])
  AC_REQUIRE([AC_HEADER_DIRENT])

])
