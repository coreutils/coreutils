/* closexec.c - set or clear the close-on-exec descriptor flag
   Copyright (C) 1991, 2004 Free Software Foundation, Inc.

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

   The code is taken from glibc/manual/llio.texi  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "cloexec.h"

#ifndef FD_CLOEXEC
# define FD_CLOEXEC 1
#endif

/* Set the `FD_CLOEXEC' flag of DESC if VALUE is nonzero,
   or clear the flag if VALUE is 0.
   Return 0 on success, or -1 on error with `errno' set. */

int
set_cloexec_flag (int desc, int value)
{
#if defined F_GETFD && defined F_SETFD

  int flags = fcntl (desc, F_GETFD, 0);

  /* If reading the flags failed, return error indication. */
  if (flags == -1)
    return flags;

  /* Set just the flag we want to set. */
  if (value != 0)
    flags |= FD_CLOEXEC;
  else
    flags &= ~FD_CLOEXEC;

  /* Store modified flags in the descriptor. */
  return fcntl (desc, F_SETFD, flags);

#else

  return 0;

#endif
}
