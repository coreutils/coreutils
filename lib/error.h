/* error.h -- declaration for error-reporting function
   Copyright (C) 1995 Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _error_h_
#define _error_h_

#if __GNUC__ >= 2 && defined (__GNUC_MINOR__) \
    && __GNUC_MINOR__ >= 5 && !defined (__STRICT_ANSI__)
#define PRINTF_ATTRIBUTE __attribute__ ((format (printf, 3, 4)))
#else
#define PRINTF_ATTRIBUTE /* empty */
#endif

#if __GNUC__
void error (int, int, const char *, ...) PRINTF_ATTRIBUTE ;
#else
void error ();
#endif

#endif /* _error_h_ */
