#serial 5

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN(jm_PREREQ,
[
  jm_PREREQ_ADDEXT
  jm_PREREQ_CANON_HOST
  jm_PREREQ_ERROR
  jm_PREREQ_QUOTEARG
  jm_PREREQ_READUTMP
  jm_PREREQ_REGEX
])

AC_DEFUN(jm_PREREQ_ADDEXT,
[
  dnl For addext.c.
  AC_SYS_LONG_FILE_NAMES
  AC_CHECK_FUNC(pathconf)
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

AC_DEFUN(jm_PREREQ_QUOTEARG,
[
  AC_CHECK_FUNCS(isascii mbrtowc)
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h wctype.h)
  AC_HEADER_STDC
  AC_C_BACKSLASH_A
  AM_C_PROTOTYPES
])

AC_DEFUN(jm_PREREQ_READUTMP,
[
  AC_HEADER_STDC
  AC_CHECK_HEADERS(string.h utmpx.h sys/param.h)
  AC_CHECK_FUNCS(utmpname)
  AM_C_PROTOTYPES

  utmp_includes="\
$ac_includes_default
#ifdef HAVE_UTMPX_H
# include <utmpx.h>
#else
# include <utmp.h>
#endif
"
  AC_CHECK_MEMBERS((struct utmpx.ut_user),,,[$utmp_includes])
  AC_CHECK_MEMBERS((struct utmp.ut_user),,,[$utmp_includes])
  AC_CHECK_MEMBERS((struct utmpx.ut_name),,,[$utmp_includes])
  AC_CHECK_MEMBERS((struct utmp.ut_name),,,[$utmp_includes])
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
