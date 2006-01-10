/* Compile-time assert-like macros.

   Copyright (C) 2005 Free Software Foundation, Inc.

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

/* Written by Paul Eggert and Jim Meyering.  */

#ifndef VERIFY_H
# define VERIFY_H 1

/* Each of these macros verifies that its argument R is a nonzero
   constant expression.  To be portable, R's type must be integer (or
   boolean).  Unlike assert, there is no run-time overhead.

   There are two macros, since no single macro can be used in all
   contexts in C.  verify_true (R) is for scalar contexts, where it
   may be cast to void if need be.  verify (R) is for declaration
   contexts, e.g., the top level.

   The symbols verify_error_if_negative_size__ and verify_function__
   are private to this header.  */

/* Verify requirement R at compile-time, as an integer constant expression.
   Return true.  */

# ifdef __cplusplus
template <int w>
  struct verify_type__ { unsigned int verify_error_if_negative_size__: w; };
#  define verify_true(R) \
     (!!sizeof (verify_type__<(R) ? 1 : -1>))
# else
#  define verify_true(R) \
     (!!sizeof \
      (struct { unsigned int verify_error_if_negative_size__: (R) ? 1 : -1; }))
# endif

/* Verify requirement R at compile-time, as a declaration without a
   trailing ';'.  */

# define verify(R) extern int (* verify_function__ (void)) [verify_true (R)]

#endif
