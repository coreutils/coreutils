/* xalloc.h -- malloc with out-of-memory checking

   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2003 Free Software Foundation, Inc.

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

#ifndef XALLOC_H_
# define XALLOC_H_

# include <stddef.h>

# ifndef __attribute__
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
#   define __attribute__(x)
#  endif
# endif

# ifndef ATTRIBUTE_NORETURN
#  define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
# endif

/* If this pointer is non-zero, run the specified function upon each
   allocation failure.  It is initialized to zero. */
extern void (*xalloc_fail_func) (void);

/* If XALLOC_FAIL_FUNC is undefined or a function that returns, this
   message is output.  It is translated via gettext.
   Its value is "memory exhausted".  */
extern char const xalloc_msg_memory_exhausted[];

/* This function is always triggered when memory is exhausted.  It is
   in charge of honoring the two previous items.  It exits with status
   exit_failure (defined in exitfail.h).  This is the
   function to call when one wants the program to die because of a
   memory allocation failure.  */
extern void xalloc_die (void) ATTRIBUTE_NORETURN;

void *xmalloc (size_t n);
void *xcalloc (size_t n, size_t s);
void *xrealloc (void *p, size_t n);
char *xstrdup (const char *str);

# define XMALLOC(Type, N_items) xmalloc (sizeof (Type) * (N_items))
# define XCALLOC(Type, N_items) xcalloc (sizeof (Type), N_items)
# define XREALLOC(Ptr, Type, N_items) xrealloc (Ptr, sizeof (Type) * (N_items))

/* Declare and alloc memory for VAR of type TYPE. */
# define NEW(Type, Var)  Type *(Var) = XMALLOC (Type, 1)

/* Free VAR only if non NULL. */
# define XFREE(Var)	\
   do {                 \
      if (Var)          \
        free (Var);     \
   } while (0)

/* Return a pointer to a malloc'ed copy of the array SRC of NUM elements. */
# define CCLONE(Src, Num) \
  (memcpy (xmalloc (sizeof *(Src) * (Num)), Src, sizeof *(Src) * (Num)))

/* Return a malloc'ed copy of SRC. */
# define CLONE(Src) CCLONE (Src, 1)


#endif /* !XALLOC_H_ */
