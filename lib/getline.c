/* getline.c -- Replacement for GNU C library function getline

   Copyright (C) 1993, 1996, 1997, 1998, 2000, 2003, 2004 Free
   Software Foundation, Inc.

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

/* Written by Jan Brittenson, bson@gnu.ai.mit.edu.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "getline.h"

#if ! (defined __GNU_LIBRARY__ && HAVE_GETDELIM)

# include "getndelim2.h"

ssize_t
getdelim (char **lineptr, size_t *linesize, int delimiter, FILE *stream)
{
  return getndelim2 (lineptr, linesize, 0, GETNLINE_NO_LIMIT, delimiter, EOF,
                     stream);
}
#endif

ssize_t
getline (char **lineptr, size_t *linesize, FILE *stream)
{
  return getdelim (lineptr, linesize, '\n', stream);
}
