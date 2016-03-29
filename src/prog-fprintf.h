/* prog-fprintf.h - common formating output functions and definitions
   Copyright (C) 2008-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef PROG_FPRINTF_H
# define PROG_FPRINTF_H

# include <stdio.h>

extern void prog_fprintf (FILE *fp, char const *fmt, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));

#endif
