/* Convert decimal strings with bounds checking and exit on error.

   Copyright (C) 2014-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#ifndef XDECTOINT_H_
# define XDECTOINT_H_ 1

# include <inttypes.h>

/* Flags for xnumtoimax and xnumtoumax.  They can be ORed together.  */
enum
  {
    /* If the number is less than MIN, do not diagnose the problem;
       instead, return MIN and set errno to EOVERFLOW or ERANGE.  */
    XTOINT_MIN_QUIET = 1 << 0,

    /* Likewise for the MAX argument.  */
    XTOINT_MAX_QUIET = 1 << 1,

    /* The MIN argument is imposed by the caller, not by the type of
       the result.  This causes the function to use ERANGE rather
       than EOVERFLOW behavior when issuing diagnostics or setting errno.  */
    XTOINT_MIN_RANGE = 1 << 2,

    /* Likewise for the MAX argument.  */
    XTOINT_MAX_RANGE = 1 << 3
  };

# define _DECLARE_XDECTOINT(name, type) \
  type name (char const *n_str, type min, type max, \
             char const *suffixes, char const *err, int err_exit) \
    _GL_ATTRIBUTE_NONNULL ((1, 5));
# define _DECLARE_XNUMTOINT(name, type) \
  type name (char const *n_str, int base, type min, type max, \
             char const *suffixes, char const *err, int err_exit, int flags) \
    _GL_ATTRIBUTE_NONNULL ((1, 6));

_DECLARE_XDECTOINT (xdectoimax, intmax_t)
_DECLARE_XDECTOINT (xdectoumax, uintmax_t)

_DECLARE_XNUMTOINT (xnumtoimax, intmax_t)
_DECLARE_XNUMTOINT (xnumtoumax, uintmax_t)

#endif  /* not XDECTOINT_H_ */
