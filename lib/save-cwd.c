/* save-cwd.c -- Save and restore current working directory.

   Copyright (C) 1995, 1997, 1998, 2003, 2004, 2005 Free Software
   Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "save-cwd.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "chdir-long.h"
#include "fcntl--.h"
#include "xgetcwd.h"

/* On systems without the fchdir function (WOE), pretend that open
   always returns -1 so that save_cwd resorts to using xgetcwd.
   Since chdir_long requires fchdir, use chdir instead.  */
#if !HAVE_FCHDIR
# undef open
# define open(File, Flags) (-1)
# undef fchdir
# define fchdir(Fd) (abort (), -1)
# undef chdir_long
# define chdir_long(Dir) chdir (Dir)
#endif

/* Record the location of the current working directory in CWD so that
   the program may change to other directories and later use restore_cwd
   to return to the recorded location.  This function may allocate
   space using malloc (via xgetcwd) or leave a file descriptor open;
   use free_cwd to perform the necessary free or close.  Upon failure,
   no memory is allocated, any locally opened file descriptors are
   closed;  return non-zero -- in that case, free_cwd need not be
   called, but doing so is ok.  Otherwise, return zero.

   The `raison d'etre' for this interface is that the working directory
   is sometimes inaccessible, and getcwd is not robust or as efficient.
   So, we prefer to use the open/fchdir approach, but fall back on
   getcwd if necessary.

   Some systems lack fchdir altogether: e.g., OS/2, pre-2001 Cygwin,
   SCO Xenix.  Also, SunOS 4 and Irix 5.3 provide the function, yet it
   doesn't work for partitions on which auditing is enabled.  If
   you're still using an obsolete system with these problems, please
   send email to the maintainer of this code.  */

int
save_cwd (struct saved_cwd *cwd)
{
  cwd->name = NULL;

  cwd->desc = open (".", O_RDONLY);
  if (cwd->desc < 0)
    {
      cwd->desc = open (".", O_WRONLY);
      if (cwd->desc < 0)
	{
	  cwd->name = xgetcwd ();
	  return cwd->name ? 0 : -1;
	}
    }

  return 0;
}

/* Change to recorded location, CWD, in directory hierarchy.
   Upon failure, return -1 (errno is set by chdir or fchdir).
   Upon success, return zero.  */

int
restore_cwd (const struct saved_cwd *cwd)
{
  if (0 <= cwd->desc)
    return fchdir (cwd->desc);
  else
    return chdir_long (cwd->name);
}

void
free_cwd (struct saved_cwd *cwd)
{
  if (cwd->desc >= 0)
    close (cwd->desc);
  if (cwd->name)
    free (cwd->name);
}
