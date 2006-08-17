/* Generate time strings directly to the output.  */

/* Copyright (C) 2005 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <time.h>

/* A cross between fprintf and nstrftime, that prints directly
   to the output stream, without the need for the potentially
   large buffer that nstrftime would require.

   Output to stream FP the result of formatting (according to the
   nstrftime format string, FMT) the time data, *TM, and the UTC
   and NANOSECONDS values.  */
size_t fprintftime (FILE *fp, char const *fmt, struct tm const *tm,
		    int utc, int nanoseconds);
