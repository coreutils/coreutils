#serial 1

# This is already in cvs autoconf -- what will be 2.52g.
# Define it here for those who aren't on the bleeding edge.
# FIXME: remove this file once the next autoconf release comes out.

undefine([AC_FUNC_STRNLEN])

# AC_FUNC_STRNLEN
# --------------
AC_DEFUN([AC_FUNC_STRNLEN],
[AC_CACHE_CHECK([for working strnlen], ac_cv_func_strnlen_working,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([], [[
#define S "foobar"
#define S_LEN (sizeof S - 1)

  /* At least one implementation is buggy: that of AIX 4.3.  */
  int i;
  for (i = 0; i < S_LEN + 1; ++i)
    {
      int result = i <= S_LEN ? i : S_LEN;
      if (strnlen (S, i) != result)
	exit (1);
    }
  exit (0);
]])],
               [ac_cv_func_strnlen_working=yes],
               [ac_cv_func_strnlen_working=no],
               [ac_cv_func_strnlen_working=no])])
test $ac_cv_func_strnlen_working = no && AC_LIBOBJ([strnlen])
])# AC_FUNC_STRNLEN
