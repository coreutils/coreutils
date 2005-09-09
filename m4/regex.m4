#serial 29

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005 Free
# Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

AC_PREREQ([2.50])

AC_DEFUN([gl_REGEX],
[
  AC_REQUIRE([AC_SYS_LARGEFILE]) dnl for a sufficently-wide off_t
  AC_DEFINE([_REGEX_LARGE_OFFSETS], 1,
    [Define if you want regoff_t to be at least as wide POSIX requires.])

  AC_LIBSOURCES(
    [regcomp.c, regex.c, regex.h,
     regex_internal.c, regex_internal.h, regexec.c])

  AC_ARG_WITH([included-regex],
    [AC_HELP_STRING([--without-included-regex],
		    [don't compile regex; this is the default on
		     systems with recent-enough versions of the GNU C
		     Library (use with caution on other systems)])])

  case $with_included_regex in
  yes|no) ac_use_included_regex=$with_included_regex
	;;
  '')
    # If the system regex support is good enough that it passes the the
    # following run test, then default to *not* using the included regex.c.
    # If cross compiling, assume the test would fail and use the included
    # regex.c.  The first failing regular expression is from `Spencer ere
    # test #75' in grep-2.3.
    AC_CACHE_CHECK([for working re_compile_pattern],
		   [gl_cv_func_re_compile_pattern_broken],
      [AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
	  [AC_INCLUDES_DEFAULT
	   #include <regex.h>],
	  [[static struct re_pattern_buffer regex;
	    const char *s;
	    struct re_registers regs;
	    /* Use the POSIX-compliant spelling with leading REG_,
	       rather than the traditional GNU spelling with leading RE_,
	       so that we reject older libc implementations.  */
	    re_set_syntax (REG_SYNTAX_POSIX_EGREP);
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("a[:@:>@:]b\n", 9, &regex);
	    /* This should fail with _Invalid character class name_ error.  */
	    if (!s)
	      exit (1);

	    /* This should succeed, but does not for e.g. glibc-2.1.3.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("{1", 2, &regex);

	    if (s)
	      exit (1);

	    /* The following example is derived from a problem report
	       against gawk from Jorge Stolfi <stolfi@ic.unicamp.br>.  */
	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("[an\371]*n", 7, &regex);
	    if (s)
	      exit (1);

	    /* This should match, but does not for e.g. glibc-2.2.1.  */
	    if (re_match (&regex, "an", 2, 0, &regs) != 2)
	      exit (1);

	    memset (&regex, 0, sizeof (regex));
	    s = re_compile_pattern ("x", 1, &regex);
	    if (s)
	      exit (1);

	    /* The version of regex.c in e.g. GNU libc-2.2.93 did not
	       work with a negative RANGE argument.  */
	    if (re_search (&regex, "wxy", 3, 2, -2, &regs) != 1)
	      exit (1);

	    /* The version of regex.c in older versions of gnulib
	       ignored REG_IGNORE_CASE (which was then called RE_ICASE).
	       Detect that problem too.  */
	    memset (&regex, 0, sizeof (regex));
	    re_set_syntax (REG_SYNTAX_EMACS | REG_IGNORE_CASE);
	    s = re_compile_pattern ("x", 1, &regex);
	    if (s)
	      exit (1);

	    if (re_search (&regex, "WXY", 3, 0, 3, &regs) < 0)
	      exit (1);

	    /* REG_STARTEND was added to glibc on 2004-01-15.
	       Reject older versions.  */
	    if (! REG_STARTEND)
	      exit (1);

	    /* Reject hosts whose regoff_t values are too narrow.
	       These include glibc 2.3.5 on hosts with 64-bit off_t
	       and 32-bit int, and Solaris 10 on hosts with 32-bit int
	       and _FILE_OFFSET_BITS=64.  */
	    if (sizeof (regoff_t) < sizeof (off_t))
	      exit (1);

	    exit (0);]])],
       [gl_cv_func_re_compile_pattern_broken=no],
       [gl_cv_func_re_compile_pattern_broken=yes],
       dnl When crosscompiling, assume it is broken.
       [gl_cv_func_re_compile_pattern_broken=yes])])
    ac_use_included_regex=$gl_cv_func_re_compile_pattern_broken
    ;;
  *) AC_MSG_ERROR([Invalid value for --with-included-regex: $with_included_regex])
    ;;
  esac

  if test $ac_use_included_regex = yes; then
    AC_LIBOBJ([regex])
    gl_PREREQ_REGEX
  fi
])

# Prerequisites of lib/regex.c and lib/regex_internal.c.
AC_DEFUN([gl_PREREQ_REGEX],
[
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([gl_C_RESTRICT])
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_CHECK_HEADERS_ONCE([locale.h wchar.h wctype.h])
  AC_CHECK_FUNCS_ONCE([isblank mbrtowc mempcpy wcrtomb wcscoll])
])
