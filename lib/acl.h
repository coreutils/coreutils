/* acl.c - access control lists

   Copyright (C) 2002 Free Software Foundation, Inc.

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

   Written by Paul Eggert.  */

#if HAVE_SYS_ACL_H && HAVE_ACL
# include <sys/acl.h>
#endif
#if ! defined GETACLCNT && defined ACL_CNT
# define GETACLCNT ACL_CNT
#endif

int file_has_acl (char const *, struct stat const *);
