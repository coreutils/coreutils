#serial 1                                                     -*- autoconf -*-

dnl FIXME
AC_DEFUN(jm_SYS_OFF_T_PRINTF_FORMAT,
[dnl
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_CHECK_HEADERS(string.h stdlinb.h)
  AC_CACHE_CHECK([for printf format that works with type off_t],
    jm_cv_sys_off_t_printf_format,
  [
   jm_cv_sys_off_t_printf_format=undef
   for jm_fmt in '' L ll q; do
     jm_OFF_T_FORMAT="$jm_fmt"
     export jm_OFF_T_FORMAT
     AC_TRY_RUN([
#      include <sys/types.h>
#      include <stdio.h>
#      if HAVE_STDLIB_H
#       include <stdlib.h>
#      endif
#      if HAVE_STRING_H
#       include <string.h>
#      endif
       int
       main()
       {
	 static off_t x[] = {1, 255, 65535, 99999999};
	 char buf[50], fmt[50];

	 /* this should be one of these values: "", "L", "ll", "q"  */
	 char *f = getenv ("jm_OFF_T_FORMAT");

	 sprintf (fmt, "%%%sd %%%sx %%%sx %%%sd", f, f, f, f);
	 sprintf (buf, fmt, x[0], x[1], x[2], x[3]);
	 exit (strcmp (buf, "1 ff ffff 99999999"));
       }
       ], jm_cv_sys_off_t_printf_format=$jm_fmt; break
       , dnl didn't work
       dnl Cross compiling -- you lose.  Specify it via the cache.
     )
   done

   # Die if none of the above worked.
   # FIXME: If this failure become a problem that we can't work around,
   # an alternative would be to arrange not to build od.
   if test "$jm_cv_sys_off_t_printf_format" = undef; then
     AC_MSG_ERROR(dnl
       [couldn't find a printf format that works with the type, off_t])
   fi
  ])

  AC_DEFINE_UNQUOTED(OFF_T_PRINTF_FORMAT_STRING,
		     "$jm_cv_sys_off_t_printf_format",
		     [printf format string for type off_t, without the `%'])
])
