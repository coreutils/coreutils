/* xalloc.h -- malloc with out-of-memory checking
   Copyright (C) 1990-1998, 1999 Free Software Foundation, Inc.

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

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

/* Exit value when the requested amount of memory is not available.
   It is initialized to EXIT_FAILURE, but the caller may set it to
   some other value.  */
extern int xalloc_exit_failure;

/* If this pointer is non-zero, run the specified function upon each
   allocation failure.  It is initialized to zero. */
extern void (*xalloc_fail_func) ();

/* If XALLOC_FAIL_FUNC is undefined or a function that returns, this
   message must be non-NULL.  It is translated via gettext.
   The default value is "Memory exhausted".  */
extern char *const xalloc_msg_memory_exhausted;

void *xmalloc PARAMS ((size_t n));
void *xcalloc PARAMS ((size_t n, size_t s));
void *xrealloc PARAMS ((void *p, size_t n));

# define XMALLOC(Type, N_bytes) ((Type *) xmalloc (sizeof (Type) * (N_bytes)))
# define XCALLOC(Type, N_bytes) ((Type *) xcalloc (sizeof (Type), (N_bytes)))
# define XREALLOC(Ptr, Type, N_bytes) \
  ((Type *) xrealloc ((void *) (Ptr), sizeof (Type) * (N_bytes)))

#endif /* !XALLOC_H_ */
