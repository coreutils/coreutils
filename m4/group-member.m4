#serial 4

dnl Written by Jim Meyering

AC_DEFUN([jm_FUNC_GROUP_MEMBER],
[
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
  AC_REQUIRE([AC_HEADER_STDC])
  AC_CHECK_HEADERS_ONCE(unistd.h)
  AC_REQUIRE([AC_FUNC_GETGROUPS])
])
