#serial 1

dnl From grep.

AC_DEFUN(jm_WITH_REGEX,
  [
    AC_ARG_WITH(included-regex,
		[  --without-included-regex don't compile regex],
		jm_with_regex=$withval,
		jm_with_regex=yes)

    if test "$jm_with_regex" = yes; then
      LIBOBJS="$LIBOBJS regex.o"
    fi
  ]
)
