/* Change the protections of file relative to an open directory.
   Copyright (C) 2006 Free Software Foundation, Inc.

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

#include <errno.h>

#include "dirname.h" /* solely for definition of IS_ABSOLUTE_FILE_NAME */
#include "save-cwd.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "openat-priv.h"

/* Some systems don't have ENOSYS.  */
#ifndef ENOSYS
# ifdef ENOTSUP
#  define ENOSYS ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define ENOSYS EINVAL
# endif
#endif

#ifndef HAVE_LCHMOD
# undef lchmod
# define lchmod(f,m) (errno = ENOSYS, -1)
#endif

/* Solaris 10 has no function like this.
   Invoke chmod or lchmod on file, FILE, using mode MODE, in the directory
   open on descriptor FD.  If possible, do it without changing the
   working directory.  Otherwise, resort to using save_cwd/fchdir,
   then mkdir/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.
   Note that an attempt to use a FLAG value of AT_SYMLINK_NOFOLLOW
   on a system without lchmod support causes this function to fail.  */
int
fchmodat (int fd, char const *file, mode_t mode, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return (flag == AT_SYMLINK_NOFOLLOW
	    ? lchmod (file, mode)
	    : chmod (file, mode));

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = (flag == AT_SYMLINK_NOFOLLOW
	   ? lchmod (proc_file, mode)
	   : chmod (proc_file, mode));
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  err = fchdir (fd);
  saved_errno = errno;

  if (! err)
    {
      err = (flag == AT_SYMLINK_NOFOLLOW
	     ? lchmod (file, mode)
	     : chmod (file, mode));
      saved_errno = errno;

      if (restore_cwd (&saved_cwd) != 0)
	openat_restore_fail (errno);
    }

  free_cwd (&saved_cwd);
  errno = saved_errno;
  return err;
}
