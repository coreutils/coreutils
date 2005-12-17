/* provide a replacement openat function
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering */

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
#  define __attribute__(x) /* empty */
# endif
#endif

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif

#ifndef AT_FDCWD
/* Use the same values as Solaris 9.  This shouldn't matter, but
   there's no real reason to differ.  */
# define AT_FDCWD (-3041965)
# define AT_SYMLINK_NOFOLLOW 4096
# define AT_REMOVEDIR 1
#endif

#ifdef __OPENAT_PREFIX

# undef openat
# define __OPENAT_CONCAT(x, y) x ## y
# define __OPENAT_XCONCAT(x, y) __OPENAT_CONCAT (x, y)
# define __OPENAT_ID(y) __OPENAT_XCONCAT (__OPENAT_PREFIX, y)
# define openat __OPENAT_ID (openat)
int openat (int fd, char const *file, int flags, /* mode_t mode */ ...);
int openat_permissive (int fd, char const *file, int flags, mode_t mode,
		       int *cwd_errno);
# if ! HAVE_FDOPENDIR
#  define fdopendir __OPENAT_ID (fdopendir)
# endif
DIR *fdopendir (int fd);
# define fstatat __OPENAT_ID (fstatat)
int fstatat (int fd, char const *file, struct stat *st, int flag);
# define unlinkat __OPENAT_ID (unlinkat)
int unlinkat (int fd, char const *file, int flag);

#else

# define openat_permissive(Fd, File, Flags, Mode, Cwd_errno) \
    openat (Fd, File, Flags, Mode)

#endif

int mkdirat (int fd, char const *file, mode_t mode);
void openat_restore_fail (int) ATTRIBUTE_NORETURN;
void openat_save_fail (int) ATTRIBUTE_NORETURN;
