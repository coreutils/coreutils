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
     gl_saved_libs=$LIBS
     # Link with -lm to detect binutils 2.16 bug with --as-needed; see
     # <http://lists.gnu.org/archive/html/bug-gnulib/2006-06/msg00131.html>.
     LIBS="$LIBS -lm"
     # Use long option sequences like '-z ignore' to test for the feature,
     # to forestall problems with linkers that have -z, -i, -g, -n, etc. flags.
     # GCC + binutils likes '-Wl,--as-needed'.
     # GCC + Solaris ld likes '-Wl,-z,ignore'.
     # Sun C likes '-z ignore'.
     # Don't try bare '--as-needed'; nothing likes it and the HP-UX 11.11
     # native cc issues annoying warnings and then ignores it,
     # which would cause us to incorrectly conclude that it worked.
     for gl_flags in \
	'-Wl,--as-needed' \
	'-Wl,-z,ignore' \
	'-z ignore'
     do
       LDFLAGS="$gl_flags $LDFLAGS"
       AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	 [gl_cv_ignore_unused_libraries=$gl_flags])
       LDFLAGS=$gl_saved_ldflags
       test "$gl_cv_ignore_unused_libraries" != none && break
     done
     LIBS=$gl_saved_libs])

  test "$gl_cv_ignore_unused_libraries" != none &&
    LDFLAGS="$LDFLAGS $gl_cv_ignore_unused_libraries"
])
