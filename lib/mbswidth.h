/* Determine the number of screen columns needed for a string.
   Copyright (C) 2000 Free Software Foundation, Inc.

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

#ifndef PARAMS
# if defined PROTOTYPES || defined __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

/* Optional flags to influence mbswidth/mbsnwidth behavior.  */

/* If this bit is set, assume invalid characters have width 0.
   Otherwise, return -1 upon finding an invalid or incomplete character.  */
#define MBSW_ACCEPT_INVALID 1

/* If this bit is set, assume unprintable characters have width 1.
   Otherwise, return -1 upon finding a non-printable character.  */
#define MBSW_ACCEPT_UNPRINTABLE	2

/* Returns the number of screen columns needed for STRING.  */
extern int mbswidth PARAMS ((const char *string, int flags));

/* Returns the number of screen columns needed for the NBYTES bytes
   starting at BUF.  */
extern int mbsnwidth PARAMS ((const char *buf, size_t nbytes, int flags));
