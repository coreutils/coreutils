/* closexec.c - set or clear the close-on-exec descriptor flag
   Copyright (C) 1991, 2004, 2005 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   The code is taken from glibc/manual/llio.texi  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cloexec.h"

#include <unistd.h>
#include <fcntl.h>

#ifndef FD_CLOEXEC
# define FD_CLOEXEC 1
#endif

/* Set the `FD_CLOEXEC' flag of DESC if VALUE is true,
   or clear the flag if VALUE is false.
   Return 0 on success, or -1 on error with `errno' set. */

int
set_cloexec_flag (int desc, bool value)
{
#if defined F_GETFD && defined F_SETFD

  int flags = fcntl (desc, F_GETFD, 0);

  if (0 <= flags)
    {
      int newflags = (value ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC);

      if (flags == newflags
	  || fcntl (desc, F_SETFD, newflags) != -1)
	return 0;
    }

  return -1;

#else

  return 0;

#endif
}
