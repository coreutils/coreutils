#serial 2

dnl See if there's a working, system-supplied version of the getline function.
dnl We can't just do AC_REPLACE_FUNCS(getline) because some systems
dnl have a function by that name in -linet that doesn't have anything
dnl to do with the function we need.
AC_DEFUN(AM_FUNC_GETLINE,
[dnl
  am_getline_needs_run_time_check=no
  am_cv_func_working_getline=yes
  AC_CHECK_FUNC(getline,
		dnl Found it in some library.  Verify that it works.
		am_getline_needs_run_time_check=yes,
		am_cv_func_working_getline=no)
  if test $am_getline_needs_run_time_check = yes; then
    AC_CHECK_HEADERS(string.h)
    AC_CACHE_CHECK([for working getline function], am_cv_func_working_getline,
    [echo fooN |tr -d '\012'|tr N '\012' > conftestdata
    AC_TRY_RUN([
#    include <stdio.h>
#    include <sys/types.h>
#    if HAVE_STRING_H
#     include <string.h>
#    endif
    int main ()
    { /* Based on a test program from Karl Heuer.  */
      char *line = NULL;
      size_t siz = 0;
      int len;
      FILE *in = fopen ("./conftestdata", "r");
      if (!in)
	return 1;
      len = getline (&line, &siz, in);
      exit ((len == 4 && line && strcmp (line, "foo\n") == 0) ? 0 : 1);
    }
    ], am_cv_func_working_getline=yes dnl The library version works.
    , am_cv_func_working_getline=no dnl The library version does NOT work.
    , am_cv_func_working_getline=no dnl We're cross compiling.
    )])
  fi

  if test $am_cv_func_working_getline = no; then
    LIBOBJS="$LIBOBJS getline.o"
    AC_SUBST(LIBOBJS)dnl
  fi
])
