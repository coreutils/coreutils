# acl.m4 - check for access control list (ACL) primitives

# Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert.

AC_DEFUN([AC_FUNC_ACL],
[
  AC_LIBSOURCES([acl.c, acl.h])
  AC_LIBOBJ([acl])

  dnl Prerequisites of lib/acl.c.
  AC_CHECK_HEADERS(sys/acl.h)
  if test "$ac_cv_header_sys_acl_h" = yes; then
    use_acl=1
  else
    use_acl=0
  fi
  AC_DEFINE_UNQUOTED(USE_ACL, $use_acl,
		     [Define if you want access control list support.])
  AC_CHECK_FUNCS(acl)
  ac_save_LIBS="$LIBS"
  AC_SEARCH_LIBS(acl_get_file, acl,
		 [test "$ac_cv_search_acl_get_file" = "none required" ||
		  LIB_ACL=$ac_cv_search_acl_get_file])
  AC_SUBST(LIB_ACL)
  AC_CHECK_HEADERS(acl/libacl.h)
  AC_CHECK_FUNCS(acl_get_file acl_get_fd acl_set_file acl_set_fd \
		 acl_free acl_from_mode acl_from_text acl_to_text \
		 acl_delete_def_file acl_entries acl_extended_file)
  LIBS="$ac_save_LIBS"
])
