/* Compare integer strings.

   Copyright (C) 2005-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert.  */

#include <config.h>

#include "strnumcmp-in.h"

#include <limits.h>

/* Compare strings A and B as integers without explicitly converting
   them to machine numbers, to avoid overflow problems and perhaps
   improve performance.  */

int
strintcmp (char const *a, char const *b)
{
  return numcompare (a, b, CHAR_MAX + 1, CHAR_MAX + 1);
}
