#serial 2

dnl A replacement for autoconf's AC_FUNC_MEMCMP that detects
dnl the losing memcmp on some x86 Next systems.
AC_DEFUN(jm_AC_FUNC_MEMCMP,
[AC_CACHE_CHECK([for working memcmp], jm_cv_func_memcmp_working,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<
main()
{
  /* Some versions of memcmp are not 8-bit clean.  */
  char c0 = 0x40, c1 = 0x80, c2 = 0x81;
  if (memcmp(&c0, &c2, 1) >= 0 || memcmp(&c1, &c2, 1) >= 0)
    exit (1);

  /* The Next x86 OpenStep bug shows up only when comparing 16 bytes
     or more and with at least one buffer not starting on a 4-byte boundary.
     William Lewis provided this test program.   */
  {
    char foo[21];
    char bar[21];
    int i;
    for (i = 0; i < 4; i++)
      {
	char *a = foo + i;
	char *b = bar + i;
	strcpy (a, "--------01111111");
	strcpy (b, "--------10000000");
	if (memcmp (a, b, 16) >= 0)
	  exit (1);
      }
    exit (0);
  }
}
>>,
changequote([, ])dnl
   jm_cv_func_memcmp_working=yes,
   jm_cv_func_memcmp_working=no,
   jm_cv_func_memcmp_working=no)])
test $jm_cv_func_memcmp_working = no && LIBOBJS="$LIBOBJS memcmp.o"
AC_SUBST(LIBOBJS)dnl
])

dnl If you use this macro in a package, you should
dnl add the following two lines to acconfig.h:
dnl   /* Define to rpl_memcmp if the replacement function should be used.  */
dnl   #undef memcmp
dnl
AC_DEFUN(jm_FUNC_MEMCMP,
[AC_REQUIRE([jm_AC_FUNC_MEMCMP])dnl
 if test $jm_cv_func_memcmp_working = no; then
   AC_DEFINE_UNQUOTED(memcmp, rpl_memcmp)
 fi
])
