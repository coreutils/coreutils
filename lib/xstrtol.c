/* A more useful interface to strtol.
   Copyright (C) 1995, 1996 Free Software Foundation, Inc.

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

/* Jim Meyering (meyering@na-net.ornl.gov) */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
#endif

#define NDEBUG
#include <assert.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef ULONG_MAX
# define ULONG_MAX ((unsigned long) ~(unsigned long) 0)
#endif

#ifndef LONG_MAX
# define LONG_MAX ((long int) (ULONG_MAX >> 1))
#endif

#include "xstrtol.h"

#define BKM_SCALE(x, scale_factor, error_return)			\
      do								\
	{								\
	  if ((x) > (double) __ZLONG_MAX / (scale_factor))		\
	    return (error_return);					\
	  (x) *= (scale_factor);					\
	}								\
      while (0)

__unsigned long int __strtol ();

/* FIXME: comment.  */

strtol_error
__xstrtol (s, ptr, base, val, valid_suffixes)
     const char *s;
     char **ptr;
     int base;
     __unsigned long int *val;
     const char *valid_suffixes;
{
  char *t_ptr;
  char **p;
  __unsigned long int tmp;

  assert (0 <= base && base <= 36);

  p = (ptr ? ptr : &t_ptr);

  errno = 0;
  tmp = __strtol (s, p, base);
  if (errno != 0)
    return LONGINT_OVERFLOW;
  if (*p == s)
    return LONGINT_INVALID;
  if (!valid_suffixes)
    {
      if (**p == '\0')
	{
	  *val = tmp;
	  return LONGINT_OK;
	}
      else
	return LONGINT_INVALID_SUFFIX_CHAR;
    }

  if (**p != '\0' && strchr (valid_suffixes, **p))
    {
      switch (**p)
	{
	case 'b':
	  BKM_SCALE (tmp, 512, LONGINT_OVERFLOW);
	  ++(*p);
	  break;

	case 'c':
	  ++(*p);
	  break;

	case 'B':
	case 'k':
	  BKM_SCALE (tmp, 1024, LONGINT_OVERFLOW);
	  ++(*p);
	  break;

	case 'm':
	  BKM_SCALE (tmp, 1024 * 1024, LONGINT_OVERFLOW);
	  ++(*p);
	  break;

	case 'w':
	  BKM_SCALE (tmp, 2, LONGINT_OVERFLOW);
	  ++(*p);
	  break;

	default:
	  return LONGINT_INVALID_SUFFIX_CHAR;
	  break;
	}
    }

  *val = tmp;
  return LONGINT_OK;
}

#ifdef TESTING_XSTRTO

#include <stdio.h>
#include "error.h"

char *program_name;

int
main (int argc, char** argv)
{
  strtol_error s_err;
  int i;

  program_name = argv[0];
  for (i=1; i<argc; i++)
    {
      char *p;
      __unsigned long int val;

      s_err = __xstrtol (argv[i], &p, 0, &val, "bckmw");
      if (s_err == LONGINT_OK)
	{
	  printf ("%s->%lu (%s)\n", argv[i], val, p);
	}
      else
	{
	  STRTOL_FATAL_ERROR (argv[i], "arg", s_err);
	}
    }
  exit (0);
}
#endif /* TESTING_XSTRTO */
