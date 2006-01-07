# If possible, ignore libraries that are not depended on.

dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Paul Eggert.

AC_DEFUN([gl_IGNORE_UNUSED_LIBRARIES],
[
  AC_CACHE_CHECK([for flag to ignore unused libraries],
    [gl_cv_ignore_unused_libraries],
    [gl_cv_ignore_unused_libraries=none
     AC_LINK_IFELSE([AC_LANG_PROGRAM()],
       [gl_ldd_output0=`(ldd conftest$ac_exeext) 2>/dev/null` ||
	  gl_ldd_output0=])
     if test "$gl_ldd_output0"; then
       gl_saved_ldflags=$LDFLAGS
       gl_saved_libs=$LIBS
       LIBS="$LIBS -lm"
       AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	 [gl_ldd_output1=`(ldd conftest$ac_exeext) 2>/dev/null` ||
	    gl_ldd_output1=])
       if test "$gl_ldd_output1" && test "$gl_ldd_output0" != "$gl_ldd_output1"
       then
	 for gl_flags in '-Xlinker -zignore' '-zignore'; do
	   LDFLAGS="$gl_flags $LDFLAGS"
	   AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	     [if gl_ldd_output2=`(ldd conftest$ac_exeext) 2>/dev/null` &&
		 test "$gl_ldd_output0" = "$gl_ldd_output2"; then
		gl_cv_ignore_unused_libraries=$gl_flags
	      fi])
	   LDFLAGS=$gl_saved_ldflags
	   test "gl_cv_ignore_unused_libraries" != none && break
	 done
       fi
       LIBS=$gl_saved_LIBS
     fi])

  test "$gl_cv_ignore_unused_libraries" != none &&
    LDFLAGS="$LDFLAGS $gl_cv_ignore_unused_libraries"
])
