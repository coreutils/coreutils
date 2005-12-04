/* macros used by openat-like functions
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

#include <stdio.h>
#include <string.h>
#include "alloca.h"
#include "intprops.h"

/* Set PROC_FD_FILENAME to the expansion of "/proc/self/fd/%d/%s" in
   alloca'd memory, using FD and FILE, respectively for %d and %s. */
#define BUILD_PROC_NAME(Proc_fd_filename, Fd, File)			\
  do									\
    {									\
      size_t filelen = strlen (File);					\
      static const char procfd[] = "/proc/self/fd/%d/%s";		\
      /* Buffer for the file name we are going to use.  It consists of	\
	 - the string /proc/self/fd/					\
	 - the file descriptor number					\
	 - the file name provided.					\
	 The final NUL is included in the sizeof.			\
	 Subtract 4 to account for %d and %s.  */			\
      size_t buflen = sizeof (procfd) - 4 + INT_STRLEN_BOUND (Fd) + filelen; \
      (Proc_fd_filename) = alloca (buflen);				\
      snprintf ((Proc_fd_filename), buflen, procfd, (Fd), (File));	\
    }									\
  while (0)

/* Trying to access a BUILD_PROC_NAME file will fail on systems without
   /proc support, and even on systems *with* ProcFS support.  Return
   nonzero if the failure may be legitimate, e.g., because /proc is not
   readable, or the particular .../fd/N directory is not present.  */
#define EXPECTED_ERRNO(Errno)			\
  ((Errno) == ENOTDIR || (Errno) == ENOENT	\
   || (Errno) == EPERM || (Errno) == EACCES	\
   || (Errno) == ENOSYS /* Solaris 8 */		\
   || (Errno) == EOPNOTSUPP /* FreeBSD */)
