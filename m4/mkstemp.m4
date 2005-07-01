#serial 11

# Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# On some hosts (e.g., HP-UX 10.20, SunOS 4.1.4, Solaris 2.5.1), mkstemp has a
# silly limit that it can create no more than 26 files from a given template.
# Other systems lack mkstemp altogether.
# On OSF1/Tru64 V4.0F, the system-provided mkstemp function can create
# only 32 files per process.
# On systems like the above, arrange to use the replacement function.
AC_DEFUN([gl_FUNC_MKSTEMP],
[dnl
  AC_REPLACE_FUNCS(mkstemp)
  if test $ac_cv_func_mkstemp = no; then
    gl_cv_func_mkstemp_limitations=yes
  else
    AC_CACHE_CHECK([for mkstemp limitations],
      gl_cv_func_mkstemp_limitations,
      [
        mkdir conftest.mkstemp
	AC_TRY_RUN([
#           include <stdlib.h>
#           include <unistd.h>
	    int main ()
	    {
	      int i;
	      for (i = 0; i < 70; i++)
		{
		  char template[] = "conftest.mkstemp/coXXXXXX";
		  int fd = mkstemp (template);
		  if (fd == -1)
		    exit (1);
		  close (fd);
		}
	      exit (0);
	    }
	    ],
	  gl_cv_func_mkstemp_limitations=no,
	  gl_cv_func_mkstemp_limitations=yes,
	  gl_cv_func_mkstemp_limitations=yes
	  )
        rm -rf conftest.mkstemp
      ]
    )
  fi

  if test $gl_cv_func_mkstemp_limitations = yes; then
    AC_LIBOBJ(mkstemp)
    AC_LIBOBJ(tempname)
    AC_DEFINE(mkstemp, rpl_mkstemp,
      [Define to rpl_mkstemp if the replacement function should be used.])
    gl_PREREQ_MKSTEMP
    gl_PREREQ_TEMPNAME
  fi
])

# Prerequisites of lib/mkstemp.c.
AC_DEFUN([gl_PREREQ_MKSTEMP],
[
])

# Prerequisites of lib/tempname.c.
AC_DEFUN([gl_PREREQ_TEMPNAME],
[
  AC_CHECK_HEADERS_ONCE(sys/time.h stdint.h unistd.h)
  AC_CHECK_FUNCS(__secure_getenv gettimeofday)
  AC_CHECK_DECLS_ONCE(getenv)
  AC_REQUIRE([gl_AC_TYPE_UINTMAX_T])
])
