#serial 4

# Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# On some systems, mkdir ("foo/", 0700) fails because of the trailing slash.
# On such systems, arrange to use a wrapper function that removes any
# trailing slashes.
AC_DEFUN([gl_FUNC_MKDIR_TRAILING_SLASH],
[dnl
  AC_CACHE_CHECK([whether mkdir fails due to a trailing slash],
    gl_cv_func_mkdir_trailing_slash_bug,
    [
      # Arrange for deletion of the temporary directory this test might create.
      ac_clean_files="$ac_clean_files confdir-slash"
      AC_TRY_RUN([
#       include <sys/types.h>
#       include <sys/stat.h>
#       include <stdlib.h>
	int main ()
	{
	  rmdir ("confdir-slash");
	  exit (mkdir ("confdir-slash/", 0700));
	}
	],
      gl_cv_func_mkdir_trailing_slash_bug=no,
      gl_cv_func_mkdir_trailing_slash_bug=yes,
      gl_cv_func_mkdir_trailing_slash_bug=yes
      )
    ]
  )

  if test $gl_cv_func_mkdir_trailing_slash_bug = yes; then
    AC_LIBOBJ(mkdir)
    AC_DEFINE(mkdir, rpl_mkdir,
      [Define to rpl_mkdir if the replacement function should be used.])
    gl_PREREQ_MKDIR
  fi
])

# Prerequisites of lib/mkdir.c.
AC_DEFUN([gl_PREREQ_MKDIR], [:])
