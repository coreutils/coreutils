# Check whether free (NULL) is supposed to work.

# Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

# We can't test for free (NULL) even at runtime, since it might
# happen to "work" for our test program, but not in general.  So, be
# conservative and use feature tests for relatively modern hosts,
# where free (NULL) is known to work.  This costs a bit of
# performance on some older hosts, but we can fix that later if
# needed.

AC_DEFUN([gl_FUNC_FREE],
[
  AC_CACHE_CHECK([whether free (NULL) is known to work],
    [gl_cv_func_free],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [[@%:@include <unistd.h>]],
	  [[@%:@if _POSIX_VERSION < 199009L && \
	        (defined unix || defined _unix || defined _unix_ \
	         || defined __unix || defined __unix__)
	      @%:@error "'free (NULL)' is not known to work"
	    @%:@endif]])],
       [gl_cv_func_free=yes],
       [gl_cv_func_free=no])])

  if test $gl_cv_func_free = no; then
    AC_LIBOBJ(free)
    AC_DEFINE(free, rpl_free,
      [Define to rpl_free if the replacement function should be used.])
  fi
])

# Prerequisites of lib/free.c.
AC_DEFUN([gl_PREREQ_FREE], [:])
