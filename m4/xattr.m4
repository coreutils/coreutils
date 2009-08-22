# xattr.m4 - check for Extended Attributes (Linux)

# Copyright (C) 2003, 2008-2009 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Originally written by Andreas Gruenbacher.
# http://www.suse.de/~agruen/coreutils/5.91/coreutils-xattr.diff

AC_DEFUN([gl_FUNC_XATTR],
[
  AC_ARG_ENABLE([xattr],
        AC_HELP_STRING([--disable-xattr],
                       [do not support extended attributes]),
        [use_xattr=$enableval], [use_xattr=yes])

  if test "$use_xattr" = "yes"; then
    AC_CHECK_HEADERS([attr/error_context.h attr/libattr.h])
    if test $ac_cv_header_attr_libattr_h = yes \
       && test $ac_cv_header_attr_error_context_h = yes; then
      use_xattr=1
    else
      use_xattr=0
    fi
    AC_DEFINE_UNQUOTED([USE_XATTR], [$use_xattr],
                       [Define if you want extended attribute support.])
    xattr_saved_LIBS=$LIBS
    AC_SEARCH_LIBS([attr_copy_file], [attr],
                   [test "$ac_cv_search_attr_copy_file" = "none required" ||
                      LIB_XATTR=$ac_cv_search_attr_copy_file])
    AC_CHECK_FUNCS([attr_copy_file])
    LIBS=$xattr_saved_LIBS
    AC_SUBST([LIB_XATTR])
  fi
])
