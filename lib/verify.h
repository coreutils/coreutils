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

#ifndef VERIFY_H
# define VERIFY_H 1

# ifndef verify_decl
#  define GL_CONCAT0(x, y) x##y
#  define GL_CONCAT(x, y) GL_CONCAT0 (x, y)

/* Verify requirement, R, at compile-time, as a declaration.
   The implementation uses a struct declaration whose name includes the
   expansion of __LINE__, so there is a small chance that two uses of
   verify_decl from different files will end up colliding (for example,
   f.c includes f.h and verify_decl is used on the same line in each).  */
#  define verify_decl(R) \
    struct GL_CONCAT (ct_assert_, __LINE__) { char a[(R) ? 1 : -1]; }
# endif

/* Verify requirement, R, at compile-time, as an expression.
   Unlike assert, there is no run-time overhead.  Unlike verify_decl,
   above, there is no risk of collision, since there is no declared name.
   This macro may be used in some contexts where the other may not, and
   vice versa.  Return void.  */
# undef verify
# define verify(R) ((void) sizeof (struct { char a[(R) ? 1 : -1]; }))

#endif
