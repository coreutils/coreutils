/* Get the working directory, compatibly with the GNU C Library.

   Copyright (C) 2004 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

/* Include the headers that might declare getcwd so that they will not
   cause confusion if included after this file.  */

#include <stdlib.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

/* If necessary, systematically rename identifiers so that they do not
   collide with the system function.  Renaming avoids problems with
   some compilers and linkers.  */

#ifdef __GETCWD_PREFIX
# undef getcwd
# define __GETCWD_CONCAT(x, y) x ## y
# define __GETCWD_XCONCAT(x, y) __GETCWD_CONCAT (x, y)
# define __GETCWD_ID(y) __GETCWD_XCONCAT (__GETCWD_PREFIX, y)
# define getcwd __GETCWD_ID (getcwd)
char *getcwd (char *, size_t);
#endif
