#serial 5

dnl A replacement for autoconf's macro by the same name.  This version
dnl uses `ac_lib' rather than `i' for the loop variable, but more importantly
dnl moves the ACTION-IF-FOUND ([$]3) into the inner `if'-block so that it is
dnl run only if one of the listed libraries ends up being used (and not in
dnl the `none required' case.
dnl I hope it's only temporary while we wait for that version to be fixed.
undefine([AC_SEARCH_LIBS])

# AC_SEARCH_LIBS(FUNCTION, SEARCH-LIBS,
#                [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                [OTHER-LIBRARIES])
# --------------------------------------------------------
# Search for a library defining FUNC, if it's not already available.
AC_DEFUN([AC_SEARCH_LIBS],
[
  AC_CACHE_CHECK([for library containing $1], [ac_cv_search_$1],
  [
    ac_func_search_save_LIBS=$LIBS
    ac_cv_search_$1=no
    AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1='none required'])
    if test "$ac_cv_search_$1" = no; then
      for ac_lib in $2; do
	LIBS="-l$ac_lib $5 $ac_func_search_save_LIBS"
	AC_TRY_LINK_FUNC([$1], [ac_cv_search_$1="-l$ac_lib"; break])
      done
    fi
    LIBS=$ac_func_search_save_LIBS
  ])

  if test "$ac_cv_search_$1" = no; then :
    $4
  else
    if test "$ac_cv_search_$1" = 'none required'; then :
      $4
    else
      LIBS="$ac_cv_search_$1 $LIBS"
      $3
    fi
  fi
])
