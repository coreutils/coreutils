/* Work around a bug of lstat on some systems

   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 Free
   Software Foundation, Inc.

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

/* The specification of these functions is in sys_stat.h.  But we cannot
   include this include file here, because on some systems, a
   "#define lstat lstat64" is being used, and sys_stat.h deletes this
   definition.  */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "stat-macros.h"

/* lstat works differently on Linux and Solaris systems.  POSIX (see
   `pathname resolution' in the glossary) requires that programs like
   `ls' take into consideration the fact that FILE has a trailing slash
   when FILE is a symbolic link.  On Linux and Solaris 10 systems, the
   lstat function already has the desired semantics (in treating
   `lstat ("symlink/", sbuf)' just like `lstat ("symlink/.", sbuf)',
   but on Solaris 9 and earlier it does not.

   If FILE has a trailing slash and specifies a symbolic link,
   then use stat() to get more info on the referent of FILE.
   If the referent is a non-directory, then set errno to ENOTDIR
   and return -1.  Otherwise, return stat's result.  */

int
rpl_lstat (const char *file, struct stat *sbuf)
{
  size_t len;
  int lstat_result = lstat (file, sbuf);

  if (lstat_result != 0 || !S_ISLNK (sbuf->st_mode))
    return lstat_result;

  len = strlen (file);
  if (len == 0 || file[len - 1] != '/')
    return 0;

  /* FILE refers to a symbolic link and the name ends with a slash.
     Call stat() to get info about the link's referent.  */

  /* If stat fails, then we do the same.  */
  if (stat (file, sbuf) != 0)
    return -1;

  /* If FILE references a directory, return 0.  */
  if (S_ISDIR (sbuf->st_mode))
    return 0;

  /* Here, we know stat succeeded and FILE references a non-directory.
     But it was specified via a name including a trailing slash.
     Fail with errno set to ENOTDIR to indicate the contradiction.  */
  errno = ENOTDIR;
  return -1;
}
