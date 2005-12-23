/* like chmod(2), but safer, if possible
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

#include "chmod-safer.h"

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "fcntl--.h" /* for the open->open_safer mapping */

#if !defined O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

#ifndef HAVE_FCHMOD
# define HAVE_FCHMOD false
# define fchmod(fd, mode) (-1)
#endif

/* Return true if the file corresponding to stat buffer pointer ST is of type,
   TYPE, and if its major and minor device numbers match (if appropriate:
   when the type is S_IFIFO or S_IFBLK).  */
static inline bool
same_file_type (struct stat const *st, dev_t device, mode_t type)
{
  /* The types must always match.  */
  if ( ! (st->st_mode & type))
    return false;

  /* For character and block devices, the major and minor device
     numbers must match, too.  */
  if (type & (S_IFCHR | S_IFBLK))
    return st->st_rdev == device;

  return true;
}

/* Assuming we can use open-with-O_NOFOLLOW and fchmod, set the permissions
   of FILE to MODE, using fchmod -- but fail (setting errno to EACCES) if
   anyone hard-links to FILE, replaces it with a symlink, or otherwise
   changes its type.  Return zero upon success.  */
static int
fchmod_new (char const *file, mode_t mode, dev_t device, mode_t file_type)
{
  int fail = 1;
  struct stat sb;
  int saved_errno = 0;
  int fd = open (file, O_NOFOLLOW | O_RDONLY | O_NDELAY);

  assert (O_NOFOLLOW);

  if (0 <= fd
      && fstat (fd, &sb) == 0
      /* Given the entry we've just created, if its link count is
	 not 1 or its type/device has changed, then someone may be
	 trying to do something nasty.  However, the risk of such an
	 attack is so low that it isn't worth a special diagnostic.
	 Simply skip the fchmod and set errno, so that the
	 code below reports the failure to set permissions.
	 Note that we don't check the link count if the expected
	 type is `directory'.  */
      && (((sb.st_nlink == 1 || file_type == S_IFDIR)
	   && same_file_type (&sb, device, file_type))
	  || ((errno = EACCES), 0))
      && fchmod (fd, mode) == 0)
    {
      fail = 0;
    }
  else
    {
      saved_errno = errno;
    }

  if (0 <= fd && close (fd) != 0 && saved_errno == 0)
    saved_errno = errno;

  errno = saved_errno;
  return fail;
}

/* Use a safer variant of chmod, if the underlying system facilities permit.
   This can avoid a minor race condition between when a file/device/directory
   is created and when we set its permissions.  If the system provides an
   fchmod function and if its open syscall honors the O_NOFOLLOW flag, then
   use open,fchmod,close.  Otherwise, just call chmod.  FILE and MODE are
   just as for chmod.  DEVICE is the combination (a la stat.st_rdev) of major
   and minor device numbers.  FILE_TYPE is one of the S_IF* values, e.g.,
   S_IFBLK, S_IFCHR, etc. */
int
chmod_safer (char const *file, mode_t mode, dev_t device, mode_t file_type)
{
  /* Using open and fchmod is reliable only if open honors the O_NOFOLLOW
     flag.  Otherwise, an attacker could simply replace the just-created
     entry with a symlink, and open would follow it blindly.  */
  return (HAVE_FCHMOD && O_NOFOLLOW
	  ? fchmod_new (file, mode, device, file_type)
	  : chmod      (file, mode));
}
