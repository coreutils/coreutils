#serial 3

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN(jm_PREREQ,
[
  jm_PREREQ_CANON_HOST
  jm_PREREQ_ERROR
  jm_PREREQ_QUOTEARG
  jm_PREREQ_REGEX
])

AC_DEFUN(jm_PREREQ_CANON_HOST,
[
  AC_CHECK_FUNCS(gethostbyname gethostbyaddr inet_ntoa)
  AC_CHECK_HEADERS(unistd.h string.h netdb.h sys/socket.h \
                   netinet/in.h arpa/inet.h)

  AC_HEADER_STDC
  AC_FUNC_ALLOCA
])

AC_DEFUN(jm_PREREQ_QUOTEARG,
[
  AC_CHECK_FUNCS(isascii mbrtowc)
  AC_CHECK_HEADERS(limits.h stdlib.h string.h wchar.h)
  AC_HEADER_STDC
  AC_C_BACKSLASH_A
  AM_C_PROTOTYPES
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
