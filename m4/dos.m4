# serial 1

# Define some macros required for proper operation of code in lib/*.c
# on MSDOS/Windows systems.

# From Jim Meyering.

AC_DEFUN(jm_AC_DOS,
  [
    #if defined _WIN32 || defined __WIN32__ || defined __MSDOS__
    ac_fspl_def="((Filename)[0] && (Filename)[1] == ':' ? 2 : 0)"
    ac_fspl_def=0
    AC_DEFINE_UNQUOTED([FILESYSTEM_PREFIX_LEN(Filename)], $ac_fspl_def,
      [On systems for which file names may have a so-called `drive letter'
       prefix, define this to compute the length of that prefix, including
       the colon.  Otherwise, define it to zero.])

    ac_isslash_def="((C) == '/' || (C) == '\\')"
    ac_isslash_def="((C) == '/')"
    AC_DEFINE_UNQUOTED([ISSLASH(C)], $ac_isslash_def,
      [Define to return nonzero for any character that may serve as
       a file name component separator.  On POSIX systems, it is the
       slash character.  Some other systems also accept backslash.])
  ])
