#serial 2

dnl These are the prerequisite macros for files in the lib/
dnl directories of the fileutils, sh-utils, and textutils packages.

AC_DEFUN(jm_PREREQ,
[
  jm_PREREQ_ERROR
  jm_PREREQ_REGEX
])

dnl FIXME: maybe put this in a separate file
AC_DEFUN(jm_PREREQ_REGEX,
[
  dnl FIXME: maybe provide a btowc replacement someday: solaris-2.5.1 lacks it
  AC_CHECK_FUNCS(bzero bcopy isascii btowc)
  AC_CHECK_HEADERS(alloca.h libintl.h wctype.h wchar.h)
  AC_HEADER_STDC
  AC_FUNC_ALLOCA
])
