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

/* Some pre-ANSI implementations (e.g. SunOS 4)
   need stderr defined if assertion checking is enabled.  */
#include <stdio.h>

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
bkm_scale (__unsigned long int *x, int scale_factor)
{
  __unsigned long int product = *x * scale_factor;
  if (*x != product / scale_factor)
    return 1;
  *x = product;
  return 0;
}

static int
bkm_scale_by_power (__unsigned long int *x, int base, int power)
{
  while (power--)
    if (bkm_scale (x, base))
      return 1;

  return 0;
}

/* FIXME: comment.  */

strtol_error
__xstrtol (const char *s, char **ptr, int strtol_base,
	   __unsigned long int *val, const char *valid_suffixes)
{
  char *t_ptr;
  char **p;
  __unsigned long int tmp;

  assert (0 <= strtol_base && strtol_base <= 36);

  p = (ptr ? ptr : &t_ptr);

  errno = 0;
  tmp = __strtol (s, p, strtol_base);
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
      int base = 1024;
      int suffixes = 1;
      int overflow;

      if (!strchr (valid_suffixes, **p))
	return LONGINT_INVALID_SUFFIX_CHAR;

      if (strchr (valid_suffixes, '0'))
	{
	  /* The ``valid suffix'' '0' is a special flag meaning that
	     an optional second suffix is allowed, which can change
	     the base, e.g. "100MD" for 100 megabytes decimal.  */

	  switch (p[0][1])
	    {
	    case 'B':
	      suffixes++;
	      break;

	    case 'D':
	      base = 1000;
	      suffixes++;
	      break;
	    }
	}

      switch (**p)
	{
	case 'b':
	  overflow = bkm_scale (&tmp, 512);
	  break;

	case 'B':
	  overflow = bkm_scale (&tmp, 1024);
	  break;

	case 'c':
	  overflow = 0;
	  break;

	case 'E': /* Exa */
	  overflow = bkm_scale_by_power (&tmp, base, 6);
	  break;

	case 'G': /* Giga */
	  overflow = bkm_scale_by_power (&tmp, base, 3);
	  break;

	case 'k': /* kilo */
	  overflow = bkm_scale_by_power (&tmp, base, 1);
	  break;

	case 'M': /* Mega */
	case 'm': /* 'm' is undocumented; for backward compatibility only */
	  overflow = bkm_scale_by_power (&tmp, base, 2);
	  break;

	case 'P': /* Peta */
	  overflow = bkm_scale_by_power (&tmp, base, 5);
	  break;

	case 'T': /* Tera */
	  overflow = bkm_scale_by_power (&tmp, base, 4);
	  break;

	case 'w':
	  overflow = bkm_scale (&tmp, 2);
	  break;

	case 'Y': /* Yotta */
	  overflow = bkm_scale_by_power (&tmp, base, 8);
	  break;

	case 'Z': /* Zetta */
	  overflow = bkm_scale_by_power (&tmp, base, 7);
	  break;

	default:
	  return LONGINT_INVALID_SUFFIX_CHAR;
	  break;
	}

      if (overflow)
	return LONGINT_OVERFLOW;

      (*p) += suffixes;
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
