/* provide a chdir function that tries not to fail due to ENAMETOOLONG
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

/* Written by Jim Meyering.  */

/* Include the headers that might declare chdir so that they will not
   cause confusion if included after this file.  */

#include <unistd.h>

/* If necessary, systematically rename identifiers so that they do not
   collide with the system function.  Renaming avoids problems with
   some compilers and linkers.  */

#ifdef __CHDIR_PREFIX
# undef chdir
# define __CHDIR_CONCAT(x, y) x ## y
# define __CHDIR_XCONCAT(x, y) __CHDIR_CONCAT (x, y)
# define __CHDIR_ID(y) __CHDIR_XCONCAT (__CHDIR_PREFIX, y)
# define chdir __CHDIR_ID (chdir)
int chdir (char const *dir);
#endif
