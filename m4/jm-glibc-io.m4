#serial 3

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros are available.
dnl Use only those *_unlocked macros that are declared.
dnl

AC_DEFUN(jm_FUNC_GLIBC_UNLOCKED_IO,
  [
    io_functions='clearerr_unlocked feof_unlocked ferror_unlocked
    fflush_unlocked fputc_unlocked fread_unlocked fwrite_unlocked
    getc_unlocked getchar_unlocked putc_unlocked putchar_unlocked'
    for jm_io_func in $io_functions; do
      # Check for the existence of each function only if its declared.
      # Otherwise, we'd get the Solaris5.5.1 functions that are not
      # declared, and that have been removed from Solaris5.6.  The resulting
      # 5.5.1 binaries would not run on 5.6 due to shared library differences.
      AC_CHECK_DECLS(($jm_io_func),
		     jm_declared=yes,
		     jm_declared=no,
		     [#include <stdio.h>])
      if test $jm_declared = yes; then
        AC_CHECK_FUNCS($jm_io_func)
      fi
    done
  ]
)
