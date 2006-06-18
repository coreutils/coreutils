#serial 9

# See if we need to emulate a missing ftruncate function using fcntl or chsize.

# Copyright (C) 2000, 2001, 2003-2006 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# FIXME: remove this macro, along with all uses of HAVE_FTRUNCATE in 2007,
# if the check below provokes no reports.

AC_DEFUN([gl_FUNC_FTRUNCATE],
[
  AC_REPLACE_FUNCS(ftruncate)
  if test $ac_cv_func_ftruncate = no; then
    gl_PREREQ_FTRUNCATE
    # If someone lacks ftruncate, make configure fail, and request
    # a bug report to inform us about it.
    if test x"$SKIP_FTRUNCATE_CHECK" != xyes; then
      AC_MSG_FAILURE([Your system lacks the ftruncate function.
	  Please report this, along with the output of "uname -a", to the
	  bug-coreutils@gnu.org mailing list.  To continue past this point,
	  rerun configure with SKIP_FTRUNCATE_CHECK=yes set in the environment.
	  E.g., env SKIP_FTRUNCATE_CHECK=yes ./configure])
    fi
  fi
])

# Prerequisites of lib/ftruncate.c.
AC_DEFUN([gl_PREREQ_FTRUNCATE],
[
  AC_CHECK_FUNCS(chsize)
])
