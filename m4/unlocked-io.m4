# unlocked-io.m4 serial 10

# Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 Free Software
# Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl From Jim Meyering.
dnl
dnl See if the glibc *_unlocked I/O macros or functions are available.
dnl Use only those *_unlocked macros or functions that are declared
dnl (because some of them were declared in Solaris 2.5.1 but were removed
dnl in Solaris 2.6, whereas we want binaries built on Solaris 2.5.1 to run
dnl on Solaris 2.6).

AC_DEFUN([gl_FUNC_GLIBC_UNLOCKED_IO],
[
  AC_DEFINE([USE_UNLOCKED_IO], 1,
    [Define to 1 if you want getc etc. to use unlocked I/O if available.
     Unlocked I/O can improve performance in unithreaded apps,
     but it is not safe for multithreaded apps.])

  dnl Persuade glibc and Solaris <stdio.h> to declare
  dnl fgets_unlocked(), fputs_unlocked() etc.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_DECLS_ONCE(
     [clearerr_unlocked feof_unlocked ferror_unlocked
      fflush_unlocked fgets_unlocked fputc_unlocked fputs_unlocked
      fread_unlocked fwrite_unlocked getc_unlocked
      getchar_unlocked putc_unlocked putchar_unlocked])
])
