#serial 2

dnl Derived from code in GNU grep.

AC_DEFUN(jm_WITH_REGEX,
  [
    dnl Even packages that don't use regex.c can use this macro.
    dnl Of course, for them it doesn't do anything.

    syscmd([test -f lib/regex.c])
    ifelse(sysval, 0,
      [
	AC_ARG_WITH(included-regex,
	    [  --without-included-regex don't compile regex (use with caution)],
		    jm_with_regex=$withval,
		    jm_with_regex=yes)
	if test "$jm_with_regex" = yes; then
	  LIBOBJS="$LIBOBJS regex.o"
	fi
      ],
    )
  ]
)
