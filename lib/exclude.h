/* exclude.h -- declarations for excluding file names
   Copyright 1992, 1993, 1994, 1997 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Paul Eggert <eggert@twinsun.com>  */

#ifndef __EXCLUDE_P
# if defined __STDC__ || __GNUC__
#  define __EXCLUDE_P(args) args
# else
#  define __EXCLUDE_P(args) ()
# endif
#endif

struct exclude;

struct exclude *new_exclude __EXCLUDE_P ((void));
void add_exclude __EXCLUDE_P ((struct exclude *, char const *));
int add_exclude_file __EXCLUDE_P ((struct exclude *, char const *, char));
int excluded_filename __EXCLUDE_P ((struct exclude const *, char const *));
