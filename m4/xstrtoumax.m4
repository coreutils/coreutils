#serial 1

# autoconf tests required for use of xstrtoumax.c

AC_DEFUN(jm_AC_PREREQ_XSTRTOUMAX,
[
  jm_AC_HEADER_INTTYPES_H
  AC_CHECK_HEADERS(stdlib.h)
  AC_CHECK_FUNCS(strtoull strtoumax strtouq)
])
