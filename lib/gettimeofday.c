/* Work around the bug in some systems whereby gettimeofday clobbers the
   static buffer that localtime uses for it's return value.  The gettimeofday
   function from Mac OS X 10.0.4, i.e. Darwin 1.3.7 has this problem.
   Copyright (C) 2001 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

/* Disable the definition of gettimeofday (from config.h) so we can use
   the library version.  */
#undef gettimeofday

#include <sys/types.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdlib.h>
#include "gtod.h"

static struct tm *localtime_buffer_addr;

void
GTOD_init (void)
{
  time_t t = 0;
  localtime_buffer_addr = localtime (&t);
}

/* This is a wrapper for gettimeofday.  It is used only on systems for which
   gettimeofday clobbers the static buffer used for localtime's result.

   Save and restore the contents of the buffer used for localtime's result
   around the call to gettimeofday.  */

int
rpl_gettimeofday (struct timeval *tv, struct timezone *tz)
{
  struct tm save;
  if (! localtime_buffer_addr)
    abort ();

  save = *localtime_buffer_addr;
  result = gettimeofday (tv, tz);
  *localtime_buffer_addr = save;

  return result;
}
