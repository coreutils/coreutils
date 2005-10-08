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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "openat.h"

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "dirname.h" /* solely for definition of IS_ABSOLUTE_FILE_NAME */
#include "save-cwd.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

/* Replacement for Solaris' openat function.
   <http://www.google.com/search?q=openat+site:docs.sun.com>
   Simulate it by doing save_cwd/fchdir/open/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, upon failure, set errno and return -1, as openat does.
   Upon successful completion, return a file descriptor.  */
int
rpl_openat (int fd, char const *file, int flags, ...)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int new_fd;
  mode_t mode = 0;

  if (flags & O_CREAT)
    {
      va_list arg;
      va_start (arg, flags);
      /* Use the promoted type (int), not mode_t, as second argument.  */
      mode = (mode_t) va_arg (arg, int);
      va_end (arg);
    }

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return open (file, flags, mode);

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return -1;
    }

  new_fd = open (file, flags, mode);
  saved_errno = errno;

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);

  errno = saved_errno;
  return new_fd;
}

#if !HAVE_FDOPENDIR

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=fdopendir+site:docs.sun.com>
   Simulate it by doing save_cwd/fchdir/opendir(".")/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' fdopendir.

   W A R N I N G:
   Unlike the other fd-related functions here, this one
   effectively consumes its FD parameter.  The caller should not
   close or otherwise manipulate FD if this function returns successfully.  */
DIR *
fdopendir (int fd)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  DIR *dir;

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return NULL;
    }

  dir = opendir (".");
  saved_errno = errno;

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);
  if (dir)
    close (fd);

  errno = saved_errno;
  return dir;
}

#endif

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=fstatat+site:docs.sun.com>
   Simulate it by doing save_cwd/fchdir/(stat|lstat)/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' fstatat.  */
int
fstatat (int fd, char const *file, struct stat *st, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD)
    return (flag == AT_SYMLINK_NOFOLLOW
	    ? lstat (file, st)
	    : stat (file, st));

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return -1;
    }

  err = (flag == AT_SYMLINK_NOFOLLOW
	 ? lstat (file, st)
	 : stat (file, st));
  saved_errno = errno;

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);

  errno = saved_errno;
  return err;
}

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=unlinkat+site:docs.sun.com>
   Simulate it by doing save_cwd/fchdir/(unlink|rmdir)/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' unlinkat.  */
int
unlinkat (int fd, char const *file, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD)
    return (flag == AT_REMOVEDIR ? rmdir (file) : unlink (file));

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return -1;
    }

  err = (flag == AT_REMOVEDIR ? rmdir (file) : unlink (file));
  saved_errno = errno;

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);

  errno = saved_errno;
  return err;
}
