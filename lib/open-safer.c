/* Invoke open, but avoid some glitches.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fcntl-safer.h"

#include <fcntl.h>
#include <stdarg.h>
#include "unistd-safer.h"

int
open_safer (char const *file, int flags, ...)
{
  mode_t mode = 0;

  if (flags & O_CREAT)
    {
      va_list ap;
      va_start (ap, flags);

      /* Assume mode_t promotes to int if and only if it is smaller.
	 This assumption isn't guaranteed by the C standard, but we
	 don't know of any real-world counterexamples.  */
      mode = (sizeof (mode_t) < sizeof (int)
	      ? va_arg (ap, int)
	      : va_arg (ap, mode_t));

      va_end (ap);
    }

  return fd_safer (open (file, flags, mode));
}
