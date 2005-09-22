#serial 16
# Determine whether we need the chown wrapper.

dnl Copyright (C) 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005 Free
dnl Software Foundation, Inc.

dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# chown should accept arguments of -1 for uid and gid, and it should
# dereference symlinks.  If it doesn't, arrange to use the replacement
# function.

# From Jim Meyering.

AC_DEFUN([gl_FUNC_CHOWN],
[
  AC_REQUIRE([AC_TYPE_UID_T])
  AC_REQUIRE([AC_FUNC_CHOWN])
  AC_REQUIRE([gl_FUNC_CHOWN_FOLLOWS_SYMLINK])

  if test $ac_cv_func_chown_works = no; then
    AC_DEFINE(CHOWN_FAILS_TO_HONOR_ID_OF_NEGATIVE_ONE, 1,
      [Define if chown is not POSIX compliant regarding IDs of -1.])
  fi

  # If chown has either of the above problems, then we need the wrapper.
  if test $ac_cv_func_chown_works$gl_cv_func_chown_follows_symlink = yesyes; then
    : # no wrapper needed
  else
    AC_LIBOBJ(chown)
    AC_DEFINE(chown, rpl_chown,
      [Define to rpl_chown if the replacement function should be used.])
    gl_PREREQ_CHOWN
  fi
])

# Determine whether chown follows symlinks (it should).
AC_DEFUN([gl_FUNC_CHOWN_FOLLOWS_SYMLINK],
[
  AC_CACHE_CHECK(
    [whether chown(2) dereferences symlinks],
    gl_cv_func_chown_follows_symlink,
    [
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

	int
	main ()
	{
	  char const *dangling_symlink = "conftest.dangle";

	  unlink (dangling_symlink);
	  if (symlink ("conftest.no-such", dangling_symlink))
	    abort ();

	  /* Exit successfully on a conforming system,
	     i.e., where chown must fail with ENOENT.  */
	  exit ( ! (chown (dangling_symlink, getuid (), getgid ()) != 0
		    && errno == ENOENT));
	}
	]])],
	[gl_cv_func_chown_follows_symlink=yes],
	[gl_cv_func_chown_follows_symlink=no],
	[gl_cv_func_chown_follows_symlink=yes]
      )
    ]
  )

  if test $gl_cv_func_chown_follows_symlink = no; then
    AC_DEFINE(CHOWN_MODIFIES_SYMLINK, 1,
      [Define if chown modifies symlinks.])
  fi
])

# Prerequisites of lib/chown.c.
AC_DEFUN([gl_PREREQ_CHOWN],
[
  AC_CHECK_FUNC([fchown], , [AC_LIBOBJ(fchown-stub)])
])
