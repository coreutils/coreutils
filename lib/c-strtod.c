/* Convert string to double, using the C locale.

   Copyright (C) 2003 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#include "c-strtod.h"

#include <locale.h>
#include <stdlib.h>

double
c_strtod (char const *nptr, char **endptr)
{
  double r;
  setlocale (LC_NUMERIC, "C");
  r = strtod (nptr, endptr);
  setlocale (LC_NUMERIC, "");
  return r;
}
