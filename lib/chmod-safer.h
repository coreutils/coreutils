/* like chmod(2), but safer, if possible
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

/* Written by Jim Meyering.  */

#ifndef FCHMOD_SAFER_H
# define FCHMOD_SAFER_H 1

# include <sys/types.h>
# include <sys/stat.h>

int chmod_safer (char const *file, mode_t mode, dev_t device, mode_t file_type);

#endif /* FCHMOD_SAFER_H */
