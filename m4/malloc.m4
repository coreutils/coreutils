#serial 1

dnl From Jim Meyering.
dnl Determine whether malloc accepts 0 as its argument.
dnl If it doesn't, arrange to use the replacement function.
dnl
dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl  /* Define to rpl_malloc if the replacement function should be used.  */
dnl  #undef malloc
dnl

AC_DEFUN(jm_FUNC_MALLOC,
[
 if test x = y; then
   dnl This code is deliberately never run via ./configure.
   dnl FIXME: this is a gross hack to make autoheader put an entry
   dnl for this symbol in config.h.in.
   AC_CHECK_FUNCS(DONE_WORKING_MALLOC_CHECK)
 fi
 dnl xmalloc.c requires that this symbol be defined so it doesn't
 dnl mistakenly use a broken malloc -- as it might if this test were omitted.
 ac_kludge=HAVE_DONE_WORKING_MALLOC_CHECK
 AC_DEFINE_UNQUOTED($ac_kludge)

 AC_CACHE_CHECK([for working malloc], jm_cv_func_working_malloc,
  [AC_TRY_RUN([
    char *malloc ();
    int
    main ()
    {
      exit (malloc (0) ? 0 : 1);
    }
	  ],
	 jm_cv_func_working_malloc=yes,
	 jm_cv_func_working_malloc=no,
	 dnl When crosscompiling, assume malloc is broken.
	 jm_cv_func_working_malloc=no)
  ])
  if test $jm_cv_func_working_malloc = no; then
    LIBOBJS="$LIBOBJS malloc.o"
    AC_DEFINE_UNQUOTED(malloc, rpl_malloc)
  fi
])
