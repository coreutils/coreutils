/* Reentrant time functions like localtime_r.

   Copyright (C) 2003, 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert.  */

#ifndef _TIME_R_H
#define _TIME_R_H

/* Include <time.h> first, since it may declare these functions with
   signatures that disagree with POSIX, and we don't want to rename
   those declarations.  */
#include <time.h>

#if !HAVE_TIME_R_POSIX

/* Don't bother with asctime_r and ctime_r, since these functions are
   not safe (like asctime and ctime, they can overrun their 26-byte
   output buffers when given outlandish struct tm values), and we
   don't want to encourage applications to use unsafe functions.  Use
   strftime or even sprintf instead.  */

# undef gmtime_r
# undef localtime_r

# define gmtime_r rpl_gmtime_r
# define localtime_r rpl_localtime_r

/* See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/gmtime.html>.  */
struct tm *gmtime_r (time_t const * restrict, struct tm * restrict);

/* See the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/localtime.html>.  */
struct tm *localtime_r (time_t const * restrict, struct tm * restrict);
#endif

#endif
