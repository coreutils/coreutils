#serial 1

dnl From Jim Meyering.
dnl Determine whether realloc accepts 0 as its first argument.
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_realloc if the replacement function should be used.  */
dnl  #undef realloc
dnl

AC_DEFUN(jm_FUNC_REALLOC,
[
 AC_CACHE_CHECK([for working realloc], jm_cv_func_working_realloc,
  [AC_TRY_RUN([
    char *realloc ();
    int
    main ()
    {
      exit (realloc (0, 1) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_realloc=yes,
	 jm_cv_func_working_realloc=no,
	 dnl When crosscompiling, assume realloc is broken.
	 jm_cv_func_working_realloc=no)
  ])
  if test $jm_cv_func_working_realloc = no; then
    LIBOBJS="$LIBOBJS realloc.o"
    AC_DEFINE_UNQUOTED(realloc, rpl_realloc)
  fi
])
