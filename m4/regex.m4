#serial 12

dnl Initially derived from code in GNU grep.
dnl Mostly written by Jim Meyering.

dnl Usage: jm_INCLUDED_REGEX([lib/regex.c])
dnl
AC_DEFUN([jm_INCLUDED_REGEX],
  [
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
		   jm_cv_func_working_re_compile_pattern,
      AC_TRY_RUN(
[#include <stdio.h>
#include <regex.h>
	  int
	  main ()
	  {
	    static struct re_pattern_buffer regex;
	    const char *s;
	    struct re_registers regs;
	    re_set_syntax (RE_SYNTAX_POSIX_EGREP);
	    [s = re_compile_pattern ("a[[:@:>@:]]b\n", 9, &regex);]
	    /* This should fail with _Invalid character class name_ error.  */
	    if (!s)
	      exit (1);

	    /* This should succeed, but doesn't for e.g. glibc-2.1.3.  */
	    s = re_compile_pattern ("{1", 2, &regex);

	    if (s)
	      exit (1);

	    /* The following example is derived from a problem report
               against gawk from Jorge Stolfi <stolfi@ic.unicamp.br>.  */
	    s = re_compile_pattern ("[[anù]]*n", 7, &regex);
	    if (s)
	      exit (1);

	    /* This should match, but doesn't for e.g. glibc-2.2.1.  */
	    if (re_match (&regex, "an", 2, 0, &regs) != 2)
	      exit (1);

	    exit (0);
	  }
	],
	       jm_cv_func_working_re_compile_pattern=yes,
	       jm_cv_func_working_re_compile_pattern=no,
	       dnl When crosscompiling, assume it's broken.
	       jm_cv_func_working_re_compile_pattern=no))
    if test $jm_cv_func_working_re_compile_pattern = yes; then
      ac_use_included_regex=no
    fi

    test -n "$1" || AC_MSG_ERROR([missing argument])
    m4_syscmd([test -f $1])
    ifelse(m4_sysval, 0,
      [
	AC_ARG_WITH(included-regex,
	[  --without-included-regex don't compile regex; this is the default on
                          systems with version 2 of the GNU C library
                          (use with caution on other system)],
		    jm_with_regex=$withval,
		    jm_with_regex=$ac_use_included_regex)
	if test "$jm_with_regex" = yes; then
	  AC_LIBOBJ(regex)
	fi
      ],
    )
  ]
)
