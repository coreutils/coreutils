#serial 3

dnl Derived from code in GNU grep.

AC_DEFUN(jm_WITH_REGEX,
  [
    AC_REQUIRE([AM_GLIBC])

    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    syscmd([test -f lib/regex.c])
    ifelse(sysval, 0,
      [
	# By default, don't use the included regex.c on systems with glibc 2
	test "$ac_cv_glibc" = yes && default=no || default=yes

	AC_ARG_WITH(included-regex,
	[  --without-included-regex don't compile regex; this is the default on
                          systems with version 2 of the GNU C library
                          (use with caution on other system)],
		    jm_with_regex=$withval,
		    jm_with_regex=$default)
	if test "$jm_with_regex" = yes; then
	  LIBOBJS="$LIBOBJS regex.o"
	fi
      ],
    )
  ]
)
