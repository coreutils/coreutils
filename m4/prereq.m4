#serial 26

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN([jm_PREREQ],
[
  jm_PREREQ_ADDEXT
  jm_PREREQ_C_STACK
  jm_PREREQ_CANON_HOST
  jm_PREREQ_DIRNAME
  jm_PREREQ_ERROR
  jm_PREREQ_EXCLUDE
  jm_PREREQ_GETPAGESIZE
  jm_PREREQ_HARD_LOCALE
  jm_PREREQ_HASH
  jm_PREREQ_HUMAN
  jm_PREREQ_MBSWIDTH
  jm_PREREQ_MEMCHR
  jm_PREREQ_PHYSMEM
  jm_PREREQ_POSIXVER
  jm_PREREQ_QUOTEARG
  jm_PREREQ_READUTMP
  jm_PREREQ_REGEX
  jm_PREREQ_STAT
  jm_PREREQ_STRNLEN
  jm_PREREQ_TEMPNAME # called by mkstemp
  jm_PREREQ_XGETCWD
  jm_PREREQ_XREADLINK
])

AC_DEFUN([jm_PREREQ_ADDEXT],
[
  dnl For addext.c.
  AC_SYS_LONG_FILE_NAMES
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

  AC_CHECK_FUNCS(gethostbyname gethostbyaddr inet_ntoa)
  AC_CHECK_HEADERS(unistd.h string.h netdb.h sys/socket.h \
                   netinet/in.h arpa/inet.h)
])

AC_DEFUN([jm_PREREQ_DIRNAME],
[
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
])

AC_DEFUN([jm_PREREQ_EXCLUDE],
[
  AC_FUNC_FNMATCH_GNU
  AC_HEADER_STDBOOL
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
  AM_C_PROTOTYPES
])

AC_DEFUN([jm_PREREQ_HASH],
[
  AC_CHECK_HEADERS(stdlib.h)
  AC_HEADER_STDBOOL
  AC_REQUIRE([jm_CHECK_DECLS])
])

# If you use human.c, you need the following files:
# inttypes.m4 ulonglong.m4
AC_DEFUN([jm_PREREQ_HUMAN],
[
  AC_CHECK_HEADERS(limits.h stdlib.h string.h)
  AC_CHECK_DECLS([getenv])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN([jm_PREREQ_MEMCHR],
[
  AC_CHECK_HEADERS(limits.h stdlib.h bp-sym.h)
])

AC_DEFUN([jm_PREREQ_PHYSMEM],
[
  AC_CHECK_HEADERS(sys/pstat.h unistd.h)
  AC_CHECK_FUNCS(pstat_getstatic pstat_getdynamic)
])

AC_DEFUN([jm_PREREQ_POSIXVER],
[
  AC_CHECK_HEADERS(unistd.h)
  AC_CHECK_DECLS([getenv])
])

AC_DEFUN([jm_PREREQ_QUOTEARG],
[
  AC_CHECK_FUNCS(isascii iswprint)
  jm_FUNC_MBRTOWC
  AC_CHECK_HEADERS(limits.h stddef.h stdlib.h string.h wchar.h wctype.h)
  AC_HEADER_STDC
  AC_C_BACKSLASH_A
  AC_TYPE_MBSTATE_T
  AM_C_PROTOTYPES
])

AC_DEFUN([jm_PREREQ_READUTMP],
[
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h utmp.h utmpx.h sys/param.h)
  AC_CHECK_FUNCS(utmpname)
  AC_CHECK_FUNCS(utmpxname)
  AM_C_PROTOTYPES

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
    AC_CHECK_MEMBERS([struct utmpx.ut_exit],,,[$utmp_includes])
    AC_CHECK_MEMBERS([struct utmp.ut_exit],,,[$utmp_includes])
    AC_LIBOBJ(readutmp)
  fi
])

AC_DEFUN([jm_PREREQ_REGEX],
[
  dnl FIXME: Maybe provide a btowc replacement someday: solaris-2.5.1 lacks it.
  dnl FIXME: Check for wctype and iswctype, and and add -lw if necessary
  dnl to get them.
  AC_CHECK_FUNCS(bzero bcopy isascii btowc)
  AC_CHECK_HEADERS(alloca.h libintl.h wctype.h wchar.h)
  AC_HEADER_STDC
  AC_FUNC_ALLOCA
])

AC_DEFUN([jm_PREREQ_STAT],
[
  AC_CHECK_HEADERS(sys/sysmacros.h sys/statvfs.h sys/vfs.h inttypes.h)
  AC_CHECK_HEADERS(sys/param.h sys/mount.h)
  AC_CHECK_FUNCS(statvfs)
  jm_AC_TYPE_LONG_LONG

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
  AC_FUNC_STRNLEN
  AC_HEADER_STDC
  AC_CHECK_HEADERS(memory.h)
  AC_CHECK_DECLS([memchr])

  # This is necessary because automake-1.6.1 doens't understand
  # that the above use of AC_FUNC_STRNLEN means we may have to use
  # lib/strnlen.c.
  test $ac_cv_func_strnlen_working = yes \
    && AC_LIBOBJ(strnlen)
])

AC_DEFUN([jm_PREREQ_TEMPNAME],
[
  AC_HEADER_STDC
  AC_HEADER_STAT
  AC_CHECK_HEADERS(fcntl.h sys/time.h stdint.h unistd.h)
  AC_CHECK_FUNCS(__secure_getenv gettimeofday)
  AC_CHECK_DECLS([getenv])
])

AC_DEFUN([jm_PREREQ_XGETCWD],
[
  AC_C_PROTOTYPES
  AC_CHECK_HEADERS(limits.h stdlib.h sys/param.h unistd.h)
  AC_CHECK_FUNCS(getcwd)
  AC_FUNC_GETCWD_NULL
])

AC_DEFUN([jm_PREREQ_XREADLINK],
[
  AC_C_PROTOTYPES
  AC_CHECK_HEADERS(limits.h stdlib.h sys/types.h unistd.h)
])
