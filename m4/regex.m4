#serial 1

dnl From grep.

AC_DEFUN(jm_WITH_REGEX,
  [
    AC_ARG_WITH(included-regex,
		[  --without-included-regex         don't compile regex],
		USE_REGEX=$withval,
		USE_REGEX=yes)

    test "$USE_REGEX" = "yes" && LIBOBJS="$LIBOBJS regex.o"
  ]
)
