/* Target directory operands for coreutils

   Copyright 2004-2024 Free Software Foundation, Inc.

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

#include <config.h>

#define TARGETDIR_INLINE _GL_EXTERN_INLINE
#include <targetdir.h>

#include <attribute.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef O_PATH
enum { O_PATHSEARCH = O_PATH };
#else
enum { O_PATHSEARCH = O_SEARCH };
#endif

/* Must F designate the working directory?  */

ATTRIBUTE_PURE static bool
must_be_working_directory (char const *f)
{
  /* Return true for ".", "./.", ".///./", etc.  */
  while (*f++ == '.')
    {
      if (*f != '/')
        return !*f;
      while (*++f == '/')
        continue;
      if (!*f)
        return true;
    }
  return false;
}

/* Return a file descriptor open to FILE, for use in openat.
   As an optimization, return AT_FDCWD if FILE must be the working directory.
   As a side effect, possibly set *ST to the file's status.
   Fail and set errno if FILE is not a directory.
   On failure return -2 if AT_FDCWD is -1, -1 otherwise.  */

int
target_directory_operand (char const *file, struct stat *st)
{
  if (must_be_working_directory (file))
    return AT_FDCWD;

  int fd = -1;
  int try_to_open = 1;
  int stat_result;

  /* On old systems without O_DIRECTORY, like Solaris 10, check with
     stat first lest we try to open a fifo for example and hang.  */
  if (!O_DIRECTORY)
    {
      stat_result = stat (file, st);
      if (stat_result == 0)
        {
          try_to_open = S_ISDIR (st->st_mode);
          errno = ENOTDIR;
        }
      else
        {
          /* On EOVERFLOW failure, give up on checking, as there is no
             easy way to check.  This should be rare.  */
          try_to_open = errno == EOVERFLOW;
        }
    }

  if (try_to_open)
    {
      fd = open (file, O_PATHSEARCH | O_DIRECTORY);

      /* On platforms lacking O_PATH, using O_SEARCH | O_DIRECTORY to
         open an overly-protected non-directory can fail with either
         EACCES or ENOTDIR.  Prefer ENOTDIR as it makes for better
         diagnostics.  */
      if (O_PATHSEARCH == O_SEARCH && fd < 0 && errno == EACCES)
        errno = (((O_DIRECTORY ? stat (file, st) : stat_result) == 0
                  && !S_ISDIR (st->st_mode))
                 ? ENOTDIR : EACCES);
    }

  if (!O_DIRECTORY && 0 <= fd)
    {
      /* On old systems like Solaris 10 double check type,
         to ensure we've opened a directory.  */
      int err;
      if (fstat (fd, st) == 0
          ? !S_ISDIR (st->st_mode) && (err = ENOTDIR, true)
          : (err = errno) != EOVERFLOW)
        {
          close (fd);
          errno = err;
          fd = -1;
        }
    }

  return fd - (AT_FDCWD == -1 && fd < 0);
}
