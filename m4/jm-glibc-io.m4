#serial 1

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros are available.
dnl

AC_DEFUN(jm_FUNC_GLIBC_UNLOCKED_IO,
  [AC_CHECK_FUNCS(				\
    clearerr_unlocked				\
    feof_unlocked				\
    ferror_unlocked				\
    fflush_unlocked				\
    fputc_unlocked				\
    fread_unlocked				\
    fwrite_unlocked				\
    getc_unlocked				\
    getchar_unlocked				\
    putc_unlocked				\
    putchar_unlocked				\
   )
  ]
)
