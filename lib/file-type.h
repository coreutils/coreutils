/* Return a string describing the type of a file.

   Copyright (C) 1993, 1994, 2001, 2002, 2004, 2005 Free Software
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

/* Written by Paul Eggert and Jim Meyering.  */

#ifndef FILE_TYPE_H
# define FILE_TYPE_H 1

# include <sys/types.h>
# include <sys/stat.h>

char const *file_type (struct stat const *);

#endif /* FILE_TYPE_H */
