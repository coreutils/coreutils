/* Provide a stub lchown function for systems that lack it.
   Copyright (C) 1998, 1999, 2002, 2004 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "lchown.h"
#include "stat-macros.h"

/* Declare chown to avoid a warning.  Don't include unistd.h,
   because it may have a conflicting prototype for lchown.  */
int chown ();

/* Work just like chown, except when FILE is a symbolic link.
   In that case, set errno to EOPNOTSUPP and return -1.
   But if autoconf tests determined that chown modifies
   symlinks, then just call chown.  */

int
lchown (const char *file, uid_t uid, gid_t gid)
{
#if ! CHOWN_MODIFIES_SYMLINK
  struct stat stats;

  if (lstat (file, &stats) == 0 && S_ISLNK (stats.st_mode))
    {
      errno = EOPNOTSUPP;
      return -1;
    }
#endif

  return chown (file, uid, gid);
}
