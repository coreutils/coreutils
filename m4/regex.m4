#serial 1

dnl From grep.

AC_DEFUN(jm_WITH_REGEX,
  [
    AC_ARG_WITH(included-regex,
		[  --without-included-regex         don't compile regex],
		USE_REGEX=$withval,
		USE_REGEX=yes)

    if test "$USE_REGEX" = yes; then

      LIBOBJS="$LIBOBJS regex.o"

    else
      if test x = y; then
	# This code is deliberately never run via ./configure.
	# FIXME: this is a hack to make autoheader put the corresponding
	# HAVE_* undef for this symbol in config.h.in.  This saves me the
	# trouble of having to maintain the #undef in acconfig.h manually.
	AC_CHECK_FUNCS(LIBC_REGEX)
      fi
      # Defining it this way (rather than via AC_DEFINE) short-circuits the
      # autoheader check -- autoheader doesn't know it's already been taken
      # care of by the hack above.
      ac_kludge=HAVE_LIBC_REGEX
      AC_DEFINE_UNQUOTED($ac_kludge)
    fi
  ]
)
