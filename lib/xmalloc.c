/* xmalloc.c -- malloc with out of memory checking

   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 2003,
   1999, 2000, 2002, 2003 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "xalloc.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

#include "error.h"
#include "exitfail.h"

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

#ifndef HAVE_MALLOC
"you must run the autoconf test for a GNU libc compatible malloc"
#endif

#ifndef HAVE_REALLOC
"you must run the autoconf test for a GNU libc compatible realloc"
#endif

/* If non NULL, call this function when memory is exhausted. */
void (*xalloc_fail_func) (void) = 0;

/* Return true if array of N objects, each of size S, cannot exist due
   to arithmetic overflow.  S must be nonzero.  */

static inline bool
array_size_overflow (size_t n, size_t s)
{
  return SIZE_MAX / s < n;
}

/* If XALLOC_FAIL_FUNC is NULL, or does return, display this message
   before exiting when memory is exhausted.  Goes through gettext. */
char const xalloc_msg_memory_exhausted[] = N_("memory exhausted");

void
xalloc_die (void)
{
  if (xalloc_fail_func)
    (*xalloc_fail_func) ();
  error (exit_failure, 0, "%s", _(xalloc_msg_memory_exhausted));
  /* The `noreturn' cannot be given to error, since it may return if
     its first argument is 0.  To help compilers understand the
     xalloc_die does terminate, call abort.  */
  abort ();
}

/* Allocate an array of N objects, each with S bytes of memory,
   dynamically, with error checking.  S must be nonzero.  */

inline void *
xnmalloc (size_t n, size_t s)
{
  void *p;
  if (array_size_overflow (n, s) || ! (p = malloc (n * s)))
    xalloc_die ();
  return p;
}

/* Allocate N bytes of memory dynamically, with error checking.  */

void *
xmalloc (size_t n)
{
  return xnmalloc (n, 1);
}

/* Change the size of an allocated block of memory P to an array of N
   objects each of S bytes, with error checking.  S must be nonzero.  */

inline void *
xnrealloc (void *p, size_t n, size_t s)
{
  if (array_size_overflow (n, s) || ! (p = realloc (p, n * s)))
    xalloc_die ();
  return p;
}

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  */

void *
xrealloc (void *p, size_t n)
{
  return xnrealloc (p, n, 1);
}

/* Allocate S bytes of zeroed memory dynamically, with error checking.
   There's no need for xnzalloc (N, S), since it would be equivalent
   to xcalloc (N, S).  */

void *
xzalloc (size_t s)
{
  return memset (xmalloc (s), 0, s);
}

/* Allocate zeroed memory for N elements of S bytes, with error
   checking.  S must be nonzero.  */

void *
xcalloc (size_t n, size_t s)
{
  void *p;
  /* Test for overflow, since some calloc implementations don't have
     proper overflow checks.  */
  if (array_size_overflow (n, s) || ! (p = calloc (n, s)))
    xalloc_die ();
  return p;
}

/* Clone an object P of size S, with error checking.  There's no need
   for xnclone (P, N, S), since xclone (P, N * S) works without any
   need for an arithmetic overflow check.  */

void *
xclone (void const *p, size_t s)
{
  return memcpy (xmalloc (s), p, s);
}
