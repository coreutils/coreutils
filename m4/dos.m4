# serial 2

# Define some macros required for proper operation of code in lib/*.c
# on MSDOS/Windows systems.

# From Jim Meyering.

AC_DEFUN(jm_AC_DOS,
  [
    # FIXME: this is incomplete.  Add a compile-test that does something
    # like this:
    #if defined _WIN32 || defined __WIN32__ || defined __MSDOS__

    AH_VERBATIM(FILESYSTEM_PREFIX_LEN,
    [#if FILESYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX
# define FILESYSTEM_PREFIX_LEN(Filename) \
  ((Filename)[0] && (Filename)[1] == ':' ? 2 : 0)
else
# define FILESYSTEM_PREFIX_LEN(Filename) 0
#endif])

    AC_DEFINE([FILESYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX],
      [Define on systems for which file names may have a so-called
       `drive letter' prefix, define this to compute the length of that
       prefix, including the colon.])

    AH_VERBATIM(ISSLASH,
    [#if FILESYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR
# define ISSLASH(C) ((C) == '/' || (C) == '\\\\')
#else
# define ISSLASH(C) ((C) == '/')
#endif])

    AC_DEFINE([FILESYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR],
      [Define if the backslash character may also serve as a file name
       component separator.])
  ])
