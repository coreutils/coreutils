#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
long int __strtol (const char *, char **, int base);
#endif

#include <assert.h>
/* FIXME: define NDEBUG before release.  */

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long) ~(unsigned long) 0)
#endif

#ifndef LONG_MAX
#define LONG_MAX ((long int) (ULONG_MAX >> 1))
#endif

#include "xstrtol.h"

__unsigned long int __strtol ();

/* FIXME: comment.  */

strtol_error
__xstrtol (s, ptr, base, val, allow_bkm_suffix)
     const char *s;
     char **ptr;
     int base;
     __unsigned long int *val;
     int allow_bkm_suffix;
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
  if (!allow_bkm_suffix)
    {
      if (**p == '\0')
	{
	  *val = tmp;
	  return LONGINT_OK;
	}
      else
	return LONGINT_INVALID_SUFFIX_CHAR;
    }

  switch (**p)
    {
    case '\0':
      break;

#define BKM_SCALE(x, scale_factor, error_return)			\
      do								\
	{								\
	  if ((x) > (double) __ZLONG_MAX / (scale_factor))		\
	    return (error_return);					\
	  (x) *= (scale_factor);					\
	}								\
      while (0)

    case 'b':
      BKM_SCALE (tmp, 512, LONGINT_OVERFLOW);
      ++(*p);
      break;

    case 'k':
      BKM_SCALE (tmp, 1024, LONGINT_OVERFLOW);
      ++(*p);
      break;

    case 'm':
      BKM_SCALE (tmp, 1024 * 1024, LONGINT_OVERFLOW);
      ++(*p);
      break;

    default:
      return LONGINT_INVALID_SUFFIX_CHAR;
      break;
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

      s_err = __xstrtol (argv[i], &p, 0, &val, 1);
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
