#serial 24

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005 Free
# Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

AC_DEFUN([gl_REGEX],
[
  gl_INCLUDED_REGEX([lib/regex.c])
])

dnl Usage: gl_INCLUDED_REGEX([lib/regex.c])
dnl
AC_DEFUN([gl_INCLUDED_REGEX],
  [
    AC_LIBSOURCES(
      [regcomp.c, regex.c, regex.h,
       regex_internal.c, regex_internal.h, regexec.c])

    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    # Assume we'll default to using the included regex.c.
    ac_use_included_regex=yes

    # However, if the system regex support is good enough that it passes the
    # the following run test, then default to *not* using the included regex.c.
    # If cross compiling, assume the test would fail and use the included
    # regex.c.  The first failing regular expression is from `Spencer ere
    # test #75' in grep-2.3.
    AC_CACHE_CHECK([for working re_compile_pattern],
		   [gl_cv_func_working_re_compile_pattern],
      [AC_RUN_IFELSE(
	 [AC_LANG_PROGRAM(
	    [AC_INCLUDES_DEFAULT
	     #include <regex.h>],
	    [[static struct re_pattern_buffer regex;
	      const char *s;
	      struct re_registers regs;
	      re_set_syntax (RE_SYNTAX_POSIX_EGREP);
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
	       * ignored RE_ICASE.  Detect that problem too. */
	      memset (&regex, 0, sizeof (regex));
	      re_set_syntax(RE_SYNTAX_EMACS|RE_ICASE);
	      s = re_compile_pattern ("x", 1, &regex);
	      if (s)
		exit (1);

	      if (re_search (&regex, "WXY", 3, 0, 3, &regs) < 0)
		exit (1);

	      /* REG_STARTEND was added to glibc on 2004-01-15.
		 Reject older versions.  */
	      if (! REG_STARTEND)
		exit (1);

	      exit (0);]])],
	 [gl_cv_func_working_re_compile_pattern=yes],
	 [gl_cv_func_working_re_compile_pattern=no],
	 dnl When crosscompiling, assume it is broken.
	 [gl_cv_func_working_re_compile_pattern=no])])
    if test $gl_cv_func_working_re_compile_pattern = yes; then
      ac_use_included_regex=no
    fi

    test -n "$1" || AC_MSG_ERROR([missing argument])
    m4_syscmd([test -f '$1'])
    ifelse(m4_sysval, 0,
      [
	AC_ARG_WITH([included-regex],
	  [  --without-included-regex don't compile regex; this is the default on
                          systems with recent-enough versions of the GNU C
                          Library (use with caution on other systems)],
	  [gl_with_regex=$withval],
	  [gl_with_regex=$ac_use_included_regex])
	if test "X$gl_with_regex" = Xyes; then
	  AC_LIBOBJ([regex])
	  gl_PREREQ_REGEX
	fi
      ],
    )
  ]
)

# Prerequisites of lib/regex.c and lib/regex_internal.c.
AC_DEFUN([gl_PREREQ_REGEX],
[
  AC_REQUIRE([gl_C_RESTRICT])
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_CHECK_HEADERS_ONCE([locale.h wchar.h wctype.h])
  AC_CHECK_FUNCS_ONCE([isblank mbrtowc mempcpy wcrtomb wcscoll])
])
