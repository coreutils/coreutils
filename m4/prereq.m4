#serial 31

dnl We use jm_ for non Autoconf macros.
m4_pattern_forbid([^jm_[ABCDEFGHIJKLMNOPQRSTUVXYZ]])dnl
m4_pattern_forbid([^gl_[ABCDEFGHIJKLMNOPQRSTUVXYZ]])dnl

# These are the prerequisite macros for files in the lib/
# directory of the coreutils package.

AC_DEFUN([jm_PREREQ],
[
  AC_REQUIRE([jm_PREREQ_ADDEXT])

  # We don't yet use c-stack.c.
  # AC_REQUIRE([jm_PREREQ_C_STACK])

  AC_REQUIRE([jm_PREREQ_CANON_HOST])
  AC_REQUIRE([jm_PREREQ_DIRNAME])
  AC_REQUIRE([jm_PREREQ_ERROR])
  AC_REQUIRE([jm_PREREQ_EXCLUDE])
  AC_REQUIRE([jm_PREREQ_GETPAGESIZE])
  AC_REQUIRE([jm_PREREQ_HARD_LOCALE])
  AC_REQUIRE([jm_PREREQ_HASH])
  AC_REQUIRE([jm_PREREQ_HUMAN])
  AC_REQUIRE([jm_PREREQ_MBSWIDTH])
  AC_REQUIRE([jm_PREREQ_MEMCHR])
  AC_REQUIRE([jm_PREREQ_PHYSMEM])
  AC_REQUIRE([jm_PREREQ_POSIXVER])
  AC_REQUIRE([jm_PREREQ_QUOTEARG])
  AC_REQUIRE([jm_PREREQ_READUTMP])
  AC_REQUIRE([jm_PREREQ_STAT])
  AC_REQUIRE([jm_PREREQ_STRNLEN])
  AC_REQUIRE([jm_PREREQ_TEMPNAME]) # called by mkstemp
  AC_REQUIRE([jm_PREREQ_XGETCWD])
  AC_REQUIRE([jm_PREREQ_XREADLINK])
])

AC_DEFUN([jm_PREREQ_ADDEXT],
[
  dnl For addext.c.
  AC_REQUIRE([AC_SYS_LONG_FILE_NAMES])
  AC_CHECK_FUNCS(pathconf)
  AC_CHECK_HEADERS(limits.h string.h unistd.h)
])

AC_DEFUN([jm_PREREQ_CANON_HOST],
[
  dnl Add any libraries as early as possible.
  dnl In particular, inet_ntoa needs -lnsl at least on Solaris5.5.1,
  dnl so we have to add -lnsl to LIBS before checking for that function.
  AC_SEARCH_LIBS(gethostbyname, [inet nsl])

  dnl These come from -lnsl on Solaris5.5.1.
  AC_CHECK_FUNCS(gethostbyname gethostbyaddr inet_ntoa)

  AC_CHECK_HEADERS(unistd.h string.h netdb.h sys/socket.h \
                   netinet/in.h arpa/inet.h)
])

AC_DEFUN([jm_PREREQ_DIRNAME],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(string.h)
])

AC_DEFUN([jm_PREREQ_EXCLUDE],
[
  AC_REQUIRE([AC_FUNC_FNMATCH_GNU])
  AC_REQUIRE([AC_HEADER_STDBOOL])
])

AC_DEFUN([jm_PREREQ_GETPAGESIZE],
[
  AC_CHECK_FUNCS(getpagesize)
  AC_CHECK_HEADERS(OS.h unistd.h)
])

AC_DEFUN([jm_PREREQ_HARD_LOCALE],
[
  AC_CHECK_HEADERS(locale.h stdlib.h string.h)
  AC_CHECK_FUNCS(setlocale)
  AC_REQUIRE([AM_C_PROTOTYPES])
])

AC_DEFUN([jm_PREREQ_HASH],
[
  AC_CHECK_HEADERS(stdlib.h)
  AC_REQUIRE([AC_HEADER_STDBOOL])
  AC_REQUIRE([jm_CHECK_DECLS])
])

# If you use human.c, you need the following files:
# inttypes.m4 longlong.m4
AC_DEFUN([jm_PREREQ_HUMAN],
[
  AC_CHECK_HEADERS(locale.h)
  AC_CHECK_DECLS([getenv])
  AC_CHECK_FUNCS(localeconv)
  AC_REQUIRE([AC_HEADER_STDBOOL])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN([jm_PREREQ_MEMCHR],
[
  AC_CHECK_HEADERS(limits.h stdlib.h bp-sym.h)
])

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

AC_DEFUN([jm_PREREQ_PHYSMEM],
[
  AC_CHECK_HEADERS([unistd.h sys/pstat.h sys/sysmp.h sys/sysinfo.h \
    machine/hal_sysinfo.h sys/table.h sys/param.h sys/sysctl.h \
    sys/systemcfg.h])
  AC_CHECK_FUNCS(pstat_getstatic pstat_getdynamic sysmp getsysinfo sysctl table)

  AC_REQUIRE([gl_SYS__SYSTEM_CONFIGURATION])
])

AC_DEFUN([jm_PREREQ_POSIXVER],
[
  AC_CHECK_HEADERS(unistd.h)
  AC_CHECK_DECLS([getenv])
])

AC_DEFUN([jm_PREREQ_QUOTEARG],
[
  AC_CHECK_FUNCS(isascii iswprint)
  AC_REQUIRE([jm_FUNC_MBRTOWC])
  AC_REQUIRE([jm_FUNC_MEMCMP])
  AC_CHECK_HEADERS(limits.h stddef.h stdlib.h string.h wchar.h wctype.h)
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_C_BACKSLASH_A])
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  AC_REQUIRE([AM_C_PROTOTYPES])
])

AC_DEFUN([jm_PREREQ_READUTMP],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(string.h utmp.h utmpx.h sys/param.h)
  AC_CHECK_FUNCS(utmpname)
  AC_CHECK_FUNCS(utmpxname)
  AC_REQUIRE([AM_C_PROTOTYPES])

  if test $ac_cv_header_utmp_h = yes || test $ac_cv_header_utmpx_h = yes; then
    utmp_includes="\
$ac_includes_default
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#endif
#ifdef HAVE_UTMP_H
# include <utmp.h>
#endif
"
    AC_CHECK_MEMBERS([struct utmpx.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_user],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_name],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_type],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_pid],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_id],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_id],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_exit],,,[$utmp_includes])

    AC_CHECK_MEMBERS([struct utmpx.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.ut_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmpx.ut_exit.e_termination],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit.e_termination],,,[$utmp_includes])
    AC_LIBOBJ(readutmp)
  fi
])

AC_DEFUN([jm_PREREQ_STAT],
[
  AC_CHECK_HEADERS(sys/sysmacros.h sys/statvfs.h sys/vfs.h inttypes.h)
  AC_CHECK_HEADERS(sys/param.h sys/mount.h)
  AC_CHECK_FUNCS(statvfs)
  AC_REQUIRE([jm_AC_TYPE_LONG_LONG])

  statxfs_includes="\
$ac_includes_default
#if HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#if ( ! HAVE_SYS_STATVFS_H && ! HAVE_SYS_VFS_H && HAVE_SYS_MOUNT_H && HAVE_SYS_PARAM_H )
/* NetBSD 1.5.2 needs these, for the declaration of struct statfs. */
# include <sys/param.h>
# include <sys/mount.h>
#endif
"
  AC_CHECK_MEMBERS([struct statfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_basetype],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fstypename],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_type],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_fsid.__val],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namemax],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statfs.f_namelen],,,[$statxfs_includes])
  AC_CHECK_MEMBERS([struct statvfs.f_namelen],,,[$statxfs_includes])
])

AC_DEFUN([jm_PREREQ_STRNLEN],
[
  AC_REQUIRE([AC_FUNC_STRNLEN])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS(memory.h)
  AC_CHECK_DECLS([memchr])

  # This is necessary because automake-1.6.1 doesn't understand
  # that the above use of AC_FUNC_STRNLEN means we may have to use
  # lib/strnlen.c.
  test $ac_cv_func_strnlen_working = yes \
    && AC_LIBOBJ(strnlen)
])

AC_DEFUN([jm_PREREQ_TEMPNAME],
[
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_HEADER_STAT])
  AC_CHECK_HEADERS(fcntl.h sys/time.h stdint.h unistd.h)
  AC_CHECK_FUNCS(__secure_getenv gettimeofday)
  AC_CHECK_DECLS([getenv])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN([jm_PREREQ_XGETCWD],
[
  AC_REQUIRE([AC_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h sys/param.h unistd.h)
  AC_CHECK_FUNCS(getcwd)
  AC_REQUIRE([AC_FUNC_GETCWD_NULL])
])

AC_DEFUN([jm_PREREQ_XREADLINK],
[
  AC_REQUIRE([AC_C_PROTOTYPES])
  AC_CHECK_HEADERS(limits.h stdlib.h sys/types.h unistd.h)
])
