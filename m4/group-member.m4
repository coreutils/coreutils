#serial 9

# Copyright (C) 1999, 2000, 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl Written by Jim Meyering

AC_DEFUN([gl_FUNC_GROUP_MEMBER],
[
  AC_LIBSOURCES([group-member.c, group-member.h])

  dnl Persuade glibc <unistd.h> to declare group_member().
  AC_REQUIRE([AC_GNU_SOURCE])

  dnl Do this replacement check manually because I want the hyphen
  dnl (not the underscore) in the filename.
  AC_CHECK_FUNC(group_member, , [
    AC_LIBOBJ(group-member)
    gl_PREREQ_GROUP_MEMBER
  ])
])

# Prerequisites of lib/group-member.c.
AC_DEFUN([gl_PREREQ_GROUP_MEMBER],
[
  AC_REQUIRE([AC_FUNC_GETGROUPS])
])
