#serial 1

dnl This is just a wrapper function to encapsulate this kludge.
dnl Putting it in a separate file like this helps share it between
dnl different packages.
AC_DEFUN(jm_CHECK_DECLS,
[
  if test x = y; then
    dnl This code is deliberately never run via ./configure.
    dnl FIXME: this is a gross hack to make autoheader put entries
    dnl for each of these symbols in the config.h.in.
    dnl Otherwise, I'd have to update acconfig.h every time I change
    dnl this list of functions.
    AC_CHECK_FUNCS(DECL_FREE DECL_LSEEK DECL_MALLOC DECL_MEMCHR DECL_REALLOC \
		   DECL_STPCPY DECL_STRSTR)
  fi
  jm_CHECK_DECLARATIONS(free lseek malloc memchr realloc stpcpy strstr)
])
