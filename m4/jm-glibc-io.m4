#serial 7 -*- autoconf -*-

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros are available.
dnl Use only those *_unlocked macros that are declared.
dnl

AC_DEFUN([jm_FUNC_GLIBC_UNLOCKED_IO],
  [AC_CHECK_DECLS(
     [clearerr_unlocked, feof_unlocked, ferror_unlocked,
      fflush_unlocked, fgets_unlocked, fputc_unlocked, fputs_unlocked,
      fread_unlocked, fwrite_unlocked, getc_unlocked,
      getchar_unlocked, putc_unlocked, putchar_unlocked])])
