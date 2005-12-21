/* like chdir(2), but safer, if possible
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

#include "chdir-safer.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fcntl--.h" /* for the open->open_safer mapping */

/* Assuming we can use open-with-O_NOFOLLOW, open DIR and fchdir into it --
   but fail (setting errno to EACCES) if anyone replaces it with a symlink,
   or otherwise changes its type.  Return zero upon success.  */
static int
fchdir_new (char const *dir)
{
  int fail = 1;
  struct stat sb;
  int saved_errno = 0;
  int fd = open (dir, O_NOFOLLOW | O_RDONLY | O_NDELAY);

  assert (O_NOFOLLOW);

  if (0 <= fd
      && fstat (fd, &sb) == 0
      /* Given the entry we've just created, if its type has changed, then
	 someone may be trying to do something nasty.  However, the risk of
	 such an attack is so low that it isn't worth a special diagnostic.
	 Simply skip the fchdir and set errno, so that the caller can
	 report the failure.  */
      && (S_ISDIR (sb.st_mode) || ((errno = EACCES), 0))
      && fchdir (fd) == 0)
    {
      fail = 0;
    }
  else
    {
      saved_errno = errno;
    }

  if (0 < fd && close (fd) != 0 && saved_errno == 0)
    saved_errno = errno;

  errno = saved_errno;
  return fail;
}

/* Just like chmod, but don't follow symlinks.
   This can avoid a minor race condition between when a directory is created
   and when we chdir into it.  If the open syscall honors the O_NOFOLLOW flag,
   then use open,fchdir,close.  Otherwise, just call chdir.  */
int
chdir_no_follow (char const *file)
{
  /* Using open and fchmod is reliable only if open honors the O_NOFOLLOW
     flag.  Otherwise, an attacker could simply replace the just-created
     entry with a symlink, and open would follow it blindly.  */
  return (O_NOFOLLOW
	  ? fchdir_new (file)
	  : chdir      (file));
}
