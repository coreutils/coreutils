#serial 1                                                     -*- autoconf -*-

dnl FIXME
AC_DEFUN(jm_SYS_OFF_T_PRINTF_FORMAT,
[dnl
    AC_CHECK_HEADERS(string.h stdlinb.h)
    AC_CACHE_CHECK([for printf format that works with type off_t],
                   jm_cv_sys_off_t_printf_format,
    [
     for i in '' L ll q; do
       jm_OFF_T_FORMAT="$i"
       export jm_OFF_T_FORMAT
     AC_TRY_RUN([
#    include <sys/types.h>
#    include <stdio.h>
#    if HAVE_STDLIB_H
#     include <stdlib.h>
#    endif
#    if HAVE_STRING_H
#     include <string.h>
#    endif
       int
       main()
       {
	 char buf[50];
	 static off_t x[] = {1, 255, 65535, 99999999};
	 char fmt[50];

	 /* this envvar should have one of these values: "", "L", "ll", "q"  */
	 char *f = getenv ("jm_OFF_T_FORMAT");
	 sprintf (fmt, "%%%sd %%%sx %%%sx %%%sd", f, f, f, f);

	 sprintf (buf, fmt, x[0], x[1], x[2], x[3]);
	 exit (strcmp (buf, "1 ff ffff 99999999"));
       }
       ], jm_cv_sys_off_t_printf_format=yes dnl The library version works.
       , jm_cv_sys_off_t_printf_format=no dnl The library version does NOT work.
       , jm_cv_sys_off_t_printf_format=no dnl We're cross compiling.
       )
     done
    ])
  fi

  if test $am_cv_func_working_getline = no; then
    AC_LIBOBJ(getline)
  fi
])
