#serial 5

dnl Determine whether to add fnmatch.o to LIBOBJS and to
dnl define fnmatch to rpl_fnmatch.
dnl

AC_DEFUN([jm_FUNC_FNMATCH],
[
  AC_FUNC_FNMATCH
  if test $ac_cv_func_fnmatch_works = no; then
    AC_LIBOBJ(fnmatch)
    AC_DEFINE(fnmatch, rpl_fnmatch,
      [Define to rpl_fnmatch if the replacement function should be used.])
  fi
])
