/* Provide a replacement for lchmod on hosts that lack it.

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

/* Written by Paul Eggert.  */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef HAVE_LCHMOD

/* The lchmod replacement follows symbolic links.  Callers should take
   this into account; lchmod should be applied only to arguments that
   are known to not be symbolic links.  On hosts that lack lchmod,
   this can lead to race conditions between the check and the
   invocation of lchmod, but we know of no workarounds that are
   reliable in general.  You might try requesting support for lchmod
   from your operating system supplier.  */

# define lchmod chmod
#endif
