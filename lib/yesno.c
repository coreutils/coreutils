/* yesno.c -- read a yes/no response from stdin
   Copyright (C) 1990, 1998, 2001 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdio.h>
#include "unlocked-io.h"

/* Read one line from standard input
   and return nonzero if that line begins with y or Y,
   otherwise return 0. */

int rpmatch ();

int
yesno ()
{
  /* We make some assumptions here:
     a) leading white space in the response are not vital
     b) the first 128 characters of the answer are enough (the rest can
	be ignored)
     I cannot think for a situation where this is not ok.  --drepper@gnu  */
  char buf[128];
  int len = 0;
  int c;

  while ((c = getchar ()) != EOF && c != '\n')
    if ((len > 0 && len < 127) || (len == 0 && !isspace (c)))
      buf[len++] = c;
  buf[len] = '\0';

  return rpmatch (buf) == 1;
}
