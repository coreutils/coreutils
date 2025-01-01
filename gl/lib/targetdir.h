/* Target directory operands for coreutils

   Copyright 2022-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <fcntl.h>
#include <sys/stat.h>

#ifndef _GL_INLINE_HEADER_BEGIN
# error "Please include config.h first."
#endif
_GL_INLINE_HEADER_BEGIN
#ifndef TARGETDIR_INLINE
# define TARGETDIR_INLINE _GL_INLINE
#endif

/* Return a file descriptor open to FILE, for use in openat.
   As an optimization, return AT_FDCWD if FILE must be the working directory.
   As a side effect, possibly set *ST to the file's status.
   Fail and set errno if FILE is not a directory.
   On failure return -2 if AT_FDCWD is -1, -1 otherwise.  */
extern int target_directory_operand (char const *file, struct stat *st);

/* Return true if FD represents success for target_directory_operand.  */
TARGETDIR_INLINE _GL_ATTRIBUTE_PURE bool
target_dirfd_valid (int fd)
{
  return fd != -1 - (AT_FDCWD == -1);
}

_GL_INLINE_HEADER_END
