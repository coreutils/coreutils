#serial 2
AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_REQUIRE([AC_HEADER_STDC])
    AC_CHECK_HEADERS(string.h sys/param.h stddef.h)
    AC_CHECK_FUNCS(resolvepath canonicalize_file_name)
    AC_REQUIRE([AC_HEADER_STAT])
  ])
