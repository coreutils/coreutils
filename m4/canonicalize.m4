#serial 1
# This would simply be AC_REPLACE_FUNC([canonicalize_file_name])
# if the function name weren't so long.  Besides, I would rather
# not have underscores in file names.
AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    dnl FIXME: add prerequisites here
    AC_CHECK_FUNC(canonicalize_file_name, , [AC_LIBOBJ(canonicalize)])
  ])
