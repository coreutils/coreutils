/* Parse dates for touch and date.

   Copyright (C) 1998, 2003 Free Software Foundation Inc.

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

/* Yacc-based version written by Jim Kingdon and David MacKenzie.
   Rewritten by Jim Meyering.  */

#ifndef POSIXTM_H_
# define POSIXTM_H_

/* POSIX Date Syntax flags.  */
# define PDS_LEADING_YEAR 1
# define PDS_TRAILING_YEAR 2
# define PDS_CENTURY 4
# define PDS_SECONDS 8

bool posixtime (time_t *p, const char *s, unsigned int syntax_bits);

#endif
