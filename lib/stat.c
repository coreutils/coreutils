/* Work around the bug in some systems whereby stat/lstat succeeds when
   given the zero-length file name argument.  The stat/lstat from SunOS4.1.4
   has this bug.
   Copyright (C) 1997-2003 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif
#ifdef LSTAT
# include <string.h>

# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif

# ifdef STAT_MACROS_BROKEN
#  undef S_ISLNK
# endif

# ifndef S_ISLNK
#  ifdef S_IFLNK
#   define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#  else
#   define S_ISLNK(m) 0
#  endif
# endif

# ifndef HAVE_DECL_FREE
"this configure-time declaration test was not run"
# endif
# if !HAVE_DECL_FREE
void free ();
# endif

# include "xalloc.h"

/* lstat works differently on Linux and Solaris systems.  POSIX (see
   `pathname resolution' in the glossary) requires that programs like `ls'
   take into consideration the fact that FILE has a trailing slash when
   FILE is a symbolic link.  On Linux systems, the lstat function already
   has the desired semantics (in treating `lstat("symlink/",sbuf)' just like
   `lstat("symlink/.",sbuf)', but on Solaris it does not.

   If FILE has a trailing slash and specifies a symbolic link,
   then append a `.' to FILE and call lstat a second time.  */

static int
slash_aware_lstat (const char *file, struct stat *sbuf)
{
  size_t len;
  char *new_file;

  int lstat_result = lstat (file, sbuf);

  if (lstat_result != 0 || !S_ISLNK (sbuf->st_mode))
    return lstat_result;

  len = strlen (file);
  if (file[len - 1] != '/')
    return lstat_result;

  /* FILE refers to a symbolic link and the name ends with a slash.
     Append a `.' to FILE and repeat the lstat call.  */

  /* Add one for the `.' we'll append, and one more for the trailing NUL.  */
  new_file = xmalloc (len + 1 + 1);
  memcpy (new_file, file, len);
  new_file[len] = '.';
  new_file[len + 1] = 0;

  lstat_result = lstat (new_file, sbuf);
  free (new_file);

  return lstat_result;
}
#endif /* LSTAT */

/* This is a wrapper for stat/lstat.
   If FILE is the empty string, fail with errno == ENOENT.
   Otherwise, return the result of calling the real stat/lstat.

   This works around the bug in some systems whereby stat/lstat succeeds when
   given the zero-length file name argument.  The stat/lstat from SunOS4.1.4
   has this bug.  */

/* This function also provides a version of lstat with consistent semantics
   when FILE specifies a symbolic link and has a trailing slash.  */

#ifdef LSTAT
# define rpl_xstat rpl_lstat
# define xstat_return_val(F, S) slash_aware_lstat (F, S)
#else
# define rpl_xstat rpl_stat
# define xstat_return_val(F, S) stat (F, S)
#endif

int
rpl_xstat (const char *file, struct stat *sbuf)
{
  if (file && *file == 0)
    {
      errno = ENOENT;
      return -1;
    }

  return xstat_return_val (file, sbuf);
}
