/* A more useful interface to strtol.
   Copyright (C) 1995, 1996, 1998 Free Software Foundation, Inc.

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

/* Written by Jim Meyering. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#if HAVE_STRING_H
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

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			      ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) (~ (t) 0 - TYPE_MINIMUM (t))

#ifndef ULONG_MAX
# define ULONG_MAX TYPE_MAXIMUM (unsigned long int)
#endif

#ifndef LONG_MAX
# define LONG_MAX TYPE_MAXIMUM (long int)
#endif

#include "xstrtol.h"

__unsigned long int __strtol ();

static int
bkm_scale (x, scale_factor)
     __unsigned long int *x;
     int scale_factor;
{
  /* The cast to `__unsigned long int' before the cast to double is
     required to work around a bug in SunOS's /bin/cc.  */
  if (*x > (double) ((__unsigned long int) __ZLONG_MAX) / scale_factor)
    {
      return 1;
    }
  *x *= scale_factor;
  return 0;
}

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

  /* Let valid_suffixes == NULL mean `allow any suffix'.  */
  /* FIXME: update all callers except the one in tail.c changing
     last parameter NULL to `""'.  */
  if (!valid_suffixes)
    {
      *val = tmp;
      return LONGINT_OK;
    }

  if (**p != '\0')
    {
      if (!strchr (valid_suffixes, **p))
	return LONGINT_INVALID_SUFFIX_CHAR;

      switch (**p)
	{
	case 'b':
	  if (bkm_scale (&tmp, 512))
	    return LONGINT_OVERFLOW;
	  ++(*p);
	  break;

	case 'c':
	  ++(*p);
	  break;

	case 'B':
	case 'k':
	  if (bkm_scale (&tmp, 1024))
	    return LONGINT_OVERFLOW;
	  ++(*p);
	  break;

	case 'm':
	  if (bkm_scale (&tmp, 1024 * 1024))
	    return LONGINT_OVERFLOW;
	  ++(*p);
	  break;

	case 'w':
	  if (bkm_scale (&tmp, 2))
	    return LONGINT_OVERFLOW;
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

# include <stdio.h>
# include "error.h"

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
