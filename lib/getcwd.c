/* Provide a replacement for the POSIX getcwd function.
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <sys/types.h>

#include "pathmax.h"
#include "same.h"

/* Undefine getcwd here so any prototype is not redefined to be a
   prototype for rpl_getcwd.  */
#undef getcwd

/* Guess high, because that makes the test below more conservative.
   But this is a kludge, because we should really use
   pathconf (".", _PC_NAME_MAX).  But it's probably not worth the cost.  */
#define KLUDGE_POSIX_NAME_MAX 255

#define MAX_SAFE_LEN (PATH_MAX - 1 - KLUDGE_POSIX_NAME_MAX - 1)

/* This is a wrapper for getcwd.
   Some implementations (at least GNU libc 2.3.1) return non-NULL
   for a working directory name longer than PATH_MAX, yet the returned
   string is a strict prefix of the desired directory name.
   Upon such a failure, free the offending string, set errno to
   ENAMETOOLONG, and return NULL.  */

char *
rpl_getcwd (char *buf, size_t size)
{
  char *cwd = getcwd (buf, size);

  if (cwd == NULL)
    return NULL;

  if (strlen (cwd) <= MAX_SAFE_LEN || same_name (cwd, "."))
    return cwd;

  free (cwd);
  errno = ENAMETOOLONG;
  return NULL;
}
