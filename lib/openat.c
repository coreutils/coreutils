/* provide a replacement openat function
   Copyright (C) 2004 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* written by Jim Meyering */

#include <config.h>

/* Disable the definition of openat to rpl_openat (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_openat on
   most systems.  */
#undef openat

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "openat.h"

#include "error.h"
#include "save-cwd.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

// FIXME
// int openat (int fildes, const char *path, int oflag, /* mode_t mode */...);

/* Replacement for Solaris' openat function.
   <http://www.google.com/search?q=openat+site:docs.sun.com>
   Simulate it by doing save_cwd/fchdir/open/restore_cwd.
   If either the fchdir or the restore_cwd fails, then exit nonzero.  */
int
rpl_openat (int fd, char const *filename, int flags)
{
  struct saved_cwd saved_cwd;
  int saved_errno = 0;
  int new_fd;

  if (fd == AT_FDCWD || *filename == '/')
    return open (filename, flags);

  if (save_cwd (&saved_cwd) != 0)
    error (EXIT_FAILURE, errno,
	   _("openat: unable to record current working directory"));
  if (fchdir (fd) != 0)
    {
      free_cwd (&saved_cwd);
      return -1;
    }

  new_fd = open (filename, flags);
  if (new_fd < 0)
    saved_errno = errno;

  if (restore_cwd (&saved_cwd) != 0)
    error (EXIT_FAILURE, errno,
	   _("openat: unable to restore working directory"));
  errno = saved_errno;

  return new_fd;
}
