#serial 3

dnl Written by Jim Meyering

AC_DEFUN([jm_FUNC_GROUP_MEMBER],
  [
    dnl Do this replacement check manually because I want the hyphen
    dnl (not the underscore) in the filename.
    AC_CHECK_FUNC(group_member, , [AC_LIBOBJ(group-member)])
  ]
)
