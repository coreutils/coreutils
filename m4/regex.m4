## --------------------------------- ##
## Check if --with-regex was given.  ##
## --------------------------------- ##

# serial 1

# The idea is to distribute rx.[hc] and regex.[hc] together, for a while.
# The WITH_REGEX symbol (which should also be documented in acconfig.h)
# is used to decide which of regex.h or rx.h should be included in the
# application.  If `./configure --with-regex' is given (the default), the
# package will use gawk's regex.  If `./configure --without-regex', a
# check is made to see if rx is already installed, as with newer Linux'es.
# If not found, the package will use the rx from the distribution.
# If found, the package will use the system's rx which, on Linux at least,
# will result in a smaller executable file.

AC_DEFUN(AM_WITH_REGEX,
[AC_MSG_CHECKING(which of GNU rx or gawk's regex is wanted)
AC_ARG_WITH(regex,
[  --without-regex         use GNU rx in lieu of gawk's regex for matching],
[test "$withval" = yes && am_with_regex=1],
[am_with_regex=1])
if test -n "$am_with_regex"; then
  AC_MSG_RESULT(regex)
  AC_DEFINE(WITH_REGEX)
  AC_CACHE_CHECK([for GNU regex in libc], am_cv_gnu_regex,
    AC_TRY_LINK([], [extern int re_max_failures; re_max_failures = 1],
		am_cv_gnu_regex=yes, am_cv_gnu_regex=no))
  if test $am_cv_gnu_regex = no; then
    LIBOBJS="$LIBOBJS regex.o"
  fi
else
  AC_MSG_RESULT(rx)
  AC_CHECK_FUNC(re_rx_search, , [LIBOBJS="$LIBOBJS rx.o"])
fi
AC_SUBST(LIBOBJS)dnl
])
