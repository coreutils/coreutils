/* Prototypes for openat-style fd-relative SELinux functions
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#include <selinux/selinux.h>
#include <selinux/context.h>

int  getfileconat (int fd, char const *file, security_context_t *con);
int lgetfileconat (int fd, char const *file, security_context_t *con);
int  setfileconat (int fd, char const *file, security_context_t con);
int lsetfileconat (int fd, char const *file, security_context_t con);
