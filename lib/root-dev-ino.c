/* root-dev-ino.c -- get the device and inode numbers for `/'.
   Copyright (C) 2003, 2005 Free Software Foundation, Inc.

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

/* Written by Jim Meyering. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include "root-dev-ino.h"

/* Call lstat to get the device and inode numbers for `/'.
   Upon failure, return NULL.  Otherwise, set the members of
   *ROOT_D_I accordingly and return ROOT_D_I.  */
struct dev_ino *
get_root_dev_ino (struct dev_ino *root_d_i)
{
  struct stat statbuf;
  if (lstat ("/", &statbuf))
    return NULL;
  root_d_i->st_ino = statbuf.st_ino;
  root_d_i->st_dev = statbuf.st_dev;
  return root_d_i;
}
