/* gethrxtime -- get high resolution real time

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

#ifndef GETHRXTIME_H_
# define GETHRXTIME_H_ 1

# include "xtime.h"

/* Get the current time, as a count of the number of nanoseconds since
   an arbitrary epoch (e.g., the system boot time).  This clock can't
   be set, is always increasing, and is nearly linear.  */

# if HAVE_ARITHMETIC_HRTIME_T && HAVE_DECL_GETHRTIME
#  include <time.h>
static inline xtime_t gethrxtime (void) { return gethrtime (); }
# else
xtime_t gethrxtime (void);
# endif

#endif
