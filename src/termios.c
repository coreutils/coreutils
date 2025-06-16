/* termios.c -- coax out Bxxx macros from termios.h

   Copyright (C) 2025 Free Software Foundation, Inc.

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

/* This simply #includes headers which may or may not provide Bxxx
   constant macros.  This is run through the C preprocessor and defined
   macros are extracted.

   In the case where the C preprocessor isn't capable of doing so,
   the script this is fed through contains a pre-defined set of common
   constants. */

#include <config.h>

#ifdef TERMIOS_NEEDS_XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif

#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
