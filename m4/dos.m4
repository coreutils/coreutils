#serial 8

# Define some macros required for proper operation of code in lib/*.c
# on MSDOS/Windows systems.

# Copyright (C) 2000, 2001, 2004 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# From Jim Meyering.

AC_DEFUN([gl_AC_DOS],
  [
    AC_CACHE_CHECK([whether system is Windows or MSDOS], [ac_cv_win_or_dos],
      [
        AC_TRY_COMPILE([],
        [#if !defined _WIN32 && !defined __WIN32__ && !defined __MSDOS__ && !defined __CYGWIN__
neither MSDOS nor Windows
#endif],
        [ac_cv_win_or_dos=yes],
        [ac_cv_win_or_dos=no])
      ])

    if test x"$ac_cv_win_or_dos" = xyes; then
      ac_fs_accepts_drive_letter_prefix=1
      ac_fs_backslash_is_file_name_separator=1
    else
      ac_fs_accepts_drive_letter_prefix=0
      ac_fs_backslash_is_file_name_separator=0
    fi

    AH_VERBATIM(FILE_SYSTEM_PREFIX_LEN,
    [#if FILE_SYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX
# define FILE_SYSTEM_PREFIX_LEN(Filename) \
  ((Filename)[0] && (Filename)[1] == ':' ? 2 : 0)
#else
# define FILE_SYSTEM_PREFIX_LEN(Filename) 0
#endif])

    AC_DEFINE_UNQUOTED([FILE_SYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX],
      $ac_fs_accepts_drive_letter_prefix,
      [Define on systems for which file names may have a so-called
       `drive letter' prefix, define this to compute the length of that
       prefix, including the colon.])

    AH_VERBATIM(ISSLASH,
    [#if FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#else
# define ISSLASH(C) ((C) == '/')
#endif])

    AC_DEFINE_UNQUOTED([FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR],
      $ac_fs_backslash_is_file_name_separator,
      [Define if the backslash character may also serve as a file name
       component separator.])
  ])
