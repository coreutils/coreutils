#serial 18

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN(jm_PREREQ,
[
  jm_PREREQ_ADDEXT
  jm_PREREQ_CANON_HOST
  jm_PREREQ_DIRNAME
  jm_PREREQ_ERROR
  jm_PREREQ_GETPAGESIZE
  jm_PREREQ_HASH
  jm_PREREQ_HUMAN
  jm_PREREQ_MBSWIDTH
  jm_PREREQ_MEMCHR
  jm_PREREQ_QUOTEARG
  jm_PREREQ_READUTMP
  jm_PREREQ_REGEX
  jm_PREREQ_TEMPNAME # called by mkstemp
])

AC_DEFUN(jm_PREREQ_ADDEXT,
[
  dnl For addext.c.
  AC_SYS_LONG_FILE_NAMES
  AC_CHECK_FUNCS(pathconf)
  AC_CHECK_HEADERS(limits.h string.h unistd.h)
])

AC_DEFUN(jm_PREREQ_CANON_HOST,
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

AC_DEFUN(jm_PREREQ_DIRNAME,
[
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h)
])

AC_DEFUN(jm_PREREQ_GETPAGESIZE,
[
  AC_CHECK_FUNCS(getpagesize)
  AC_CHECK_HEADERS(OS.h unistd.h)
])

AC_DEFUN(jm_PREREQ_HASH,
[
  AC_CHECK_HEADERS(stdlib.h stdbool.h)
  AC_REQUIRE([jm_CHECK_DECLS])
])

# If you use human.c, you need the following files:
# uintmax_t.m4 inttypes_h.m4 ulonglong.m4
AC_DEFUN(jm_PREREQ_HUMAN,
[
  AC_CHECK_HEADERS(limits.h stdlib.h string.h)
  AC_CHECK_DECLS([getenv])
  AC_REQUIRE([jm_AC_HEADER_INTTYPES_H])
  AC_REQUIRE([jm_AC_TYPE_UINTMAX_T])
])

AC_DEFUN(jm_PREREQ_MEMCHR,
[
  AC_CHECK_HEADERS(limits.h stdlib.h bp-sym.h)
])

AC_DEFUN(jm_PREREQ_QUOTEARG,
[
  AC_CHECK_FUNCS(isascii iswprint)
  jm_FUNC_MBRTOWC
  AC_CHECK_HEADERS(limits.h stddef.h stdlib.h string.h wchar.h wctype.h)
  AC_HEADER_STDC
  AC_C_BACKSLASH_A
  AC_MBSTATE_T
  AM_C_PROTOTYPES
])

AC_DEFUN(jm_PREREQ_READUTMP,
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
    AC_LIBOBJ(readutmp)
  fi
])

AC_DEFUN(jm_PREREQ_REGEX,
[
  dnl FIXME: Maybe provide a btowc replacement someday: solaris-2.5.1 lacks it.
  dnl FIXME: Check for wctype and iswctype, and and add -lw if necessary
  dnl to get them.
  AC_CHECK_FUNCS(bzero bcopy isascii btowc)
  AC_CHECK_HEADERS(alloca.h libintl.h wctype.h wchar.h)
  AC_HEADER_STDC
  AC_FUNC_ALLOCA
])

AC_DEFUN(jm_PREREQ_TEMPNAME,
[
  AC_HEADER_STDC
  AC_HEADER_STAT
  AC_CHECK_HEADERS(fcntl.h sys/time.h stdint.h unistd.h)
  AC_CHECK_FUNCS(__secure_getenv gettimeofday)
])
