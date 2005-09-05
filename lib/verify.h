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

# define GL_CONCAT0(x, y) x##y
# define GL_CONCAT(x, y) GL_CONCAT0 (x, y)

/* If gcc predates 3.0, then disable the check below to ensure
   that verify_type__'s argument is a constant expression.  */
# if __GNUC__ <= 2
#  defined __builtin_constant_p(R) 1
# endif

/* A type that is valid if and only if R is nonzero.
   R should be an integer constant expression.
   verify_type__ and verify_error_if_negative_size__ are symbols that
   are private to this header file.  */

# define verify_type__(R) \
    struct { \
      /* Provoke a compile-time failure if R is a non-constant expression. */ \
      int verify_error_if_non_const__[__builtin_constant_p (R) ? 1 : -1]; \
      /* Provoke a compile-time failure if R is nonzero.  */ \
      int verify_error_if_negative_size__[(R) ? 1 : -1]; }

/* Verify requirement R at compile-time, as a declaration.
   R should be an integer constant expression.
   Unlike assert, there is no run-time overhead.

   The implementation uses __LINE__ to lessen the probability of
   generating a warning that verify_function_NNN is multiply declared.
   However, even if two declarations in different files have the same
   __LINE__, the multiple declarations are still valid C89 and C99
   code because they simply redeclare the same external function, so
   no conforming compiler will reject them outright.  */

# define verify(R) \
    extern verify_type__ (R) GL_CONCAT (verify_function_, __LINE__) (void)

/* Verify requirement R at compile-time, as an expression.
   R should be an integer constant expression.
   Unlike assert, there is no run-time overhead.
   This macro can be used in some contexts where verify cannot, and vice versa.
   Return void.  */

# define verify_expr(R) ((void) ((verify_type__ (R) *) 0))

#endif
