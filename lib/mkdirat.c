/* provide a replacement openat function
   Copyright (C) 2005 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "dirname.h" /* solely for definition of IS_ABSOLUTE_FILE_NAME */
#include "save-cwd.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "openat-priv.h"

/* Solaris 10 has no function like this.
   Create a subdirectory, FILE, with mode MODE, in the directory
   open on descriptor FD.  If possible, do it without changing the
   working directory.  Otherwise, resort to using save_cwd/fchdir,
   then mkdir/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.  */
int
mkdirat (int fd, char const *file, mode_t mode)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return mkdir (file, mode);

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = mkdir (proc_file, mode);
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return -1;
    }

  err = mkdir (file, mode);
  saved_errno = (err < 0 ? errno : 0);

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);

  if (saved_errno)
    errno = saved_errno;
  return err;
}
