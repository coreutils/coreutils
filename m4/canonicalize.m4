#serial 1
AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_REQUIRE([AC_HEADER_STDC])
    AC_CHECK_HEADERS(string.h sys/param.h stddef.h)
    AC_CHECK_FUNCS(resolvepath)
    AC_REQUIRE([AC_HEADER_STAT])

    # This would simply be AC_REPLACE_FUNC([canonicalize_file_name])
    # if the function name weren't so long.  Besides, I would rather
    # not have underscores in file names.
    AC_CHECK_FUNC([canonicalize_file_name], , [AC_LIBOBJ(canonicalize)])
  ])
