/* acl.c - access control lists

   Copyright (C) 2002 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Paul Eggert.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/stat.h>
#ifndef S_ISLNK
# define S_ISLNK(Mode) 0
#endif

#include "acl.h"

#include <errno.h>
#ifndef ENOSYS
# define ENOSYS (-1)
#endif

#ifndef MIN_ACL_ENTRIES
# define MIN_ACL_ENTRIES 4
#endif

/* Return 1 if PATH has a nontrivial access control list, 0 if not,
   and -1 (setting errno) if an error is encountered.  */

int
file_has_acl (char const *path, struct stat const *pathstat)
{
  /* FIXME: This implementation should work on recent-enough versions
     of HP-UX, Solaris, and Unixware, but it simply returns 0 with
     POSIX 1003.1e (draft 17 -- abandoned), AIX, GNU/Linux, Irix, and
     Tru64.  Please see Samba's source/lib/sysacls.c file for
     fix-related ideas.  */

#if HAVE_ACL && defined GETACLCNT
  if (! S_ISLNK (pathstat->st_mode))
    {
      int n = acl (path, GETACLCNT, 0, NULL);
      return n < 0 ? (errno == ENOSYS ? 0 : -1) : (MIN_ACL_ENTRIES < n);
    }
#endif

  return 0;
}
