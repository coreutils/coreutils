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
     gl_saved_ldflags=$LDFLAGS
     # Use long option sequences like '-z ignore' to test for the feature,
     # to forestall problems with linkers that have -z, -i, -g, -n, etc. flags.
     for gl_flags in '-Wl,-z,ignore' '-z ignore'; do
       LDFLAGS="$gl_flags $LDFLAGS"
       AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	 [gl_cv_ignore_unused_libraries=$gl_flags])
       LDFLAGS=$gl_saved_ldflags
       test "$gl_cv_ignore_unused_libraries" != none && break
     done])

  test "$gl_cv_ignore_unused_libraries" != none &&
    LDFLAGS="$LDFLAGS $gl_cv_ignore_unused_libraries"
])
