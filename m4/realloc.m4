#serial 1

dnl From Jim Meyering.
dnl Determine whether realloc works when both arguments are 0.
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_realloc if the replacement function should be used.  */
dnl  #undef realloc
dnl

AC_DEFUN(jm_FUNC_REALLOC,
[
 if test x = y; then
   dnl This code is deliberately never run via ./configure.
   dnl FIXME: this is a gross hack to make autoheader put an entry
   dnl for this symbol in config.h.in.
   AC_CHECK_FUNCS(DONE_WORKING_REALLOC_CHECK)
 fi
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken realloc -- as it might if this test were omitted.
 ac_kludge=HAVE_DONE_WORKING_REALLOC_CHECK
 AC_DEFINE_UNQUOTED($ac_kludge)

 AC_CACHE_CHECK([for working realloc], jm_cv_func_working_realloc,
  [AC_TRY_RUN([
    char *realloc ();
    int
    main ()
    {
      exit (realloc (0, 0) ? 0 : 1);
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
