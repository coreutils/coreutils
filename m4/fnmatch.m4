#serial 2

dnl Determine whether to add fnmatch.o to LIBOBJS and to
dnl define fnmatch to rpl_fnmatch.
dnl

AC_DEFUN(jm_FUNC_FNMATCH,
[
  AC_REQUIRE([AM_GLIBC])
  AC_FUNC_FNMATCH
  if test $ac_cv_func_fnmatch_works = no \
      && test $ac_cv_gnu_library = no; then
    LIBOBJS="$LIBOBJS fnmatch.o"
    AC_DEFINE_UNQUOTED(fnmatch, rpl_fnmatch,
      [Define to rpl_fnmatch if the replacement function should be used.])
  fi
])
