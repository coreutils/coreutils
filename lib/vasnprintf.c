/* vsprintf with automatic memory allocation.
   Copyright (C) 1999, 2002-2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Tell glibc's <stdio.h> to provide a prototype for snprintf().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifndef IN_LIBINTL
# include <alloca.h>
#endif

/* Specification.  */
#if WIDE_CHAR_VERSION
# include "vasnwprintf.h"
#else
# include "vasnprintf.h"
#endif

#include <stdio.h>	/* snprintf(), sprintf() */
#include <stdlib.h>	/* abort(), malloc(), realloc(), free() */
#include <string.h>	/* memcpy(), strlen() */
#include <errno.h>	/* errno */
#include <limits.h>	/* CHAR_BIT, INT_MAX */
#include <float.h>	/* DBL_MAX_EXP, LDBL_MAX_EXP */
#if WIDE_CHAR_VERSION
# include "wprintf-parse.h"
#else
# include "printf-parse.h"
#endif

#ifndef SIZE_MAX
# define SIZE_MAX ((size_t) -1)
#endif

/* Some systems, like OSF/1 4.0 and Woe32, don't have EOVERFLOW.  */
#ifndef EOVERFLOW
# define EOVERFLOW E2BIG
#endif

#ifdef HAVE_WCHAR_T
# ifdef HAVE_WCSLEN
#  define local_wcslen wcslen
# else
   /* Solaris 2.5.1 has wcslen() in a separate library libw.so. To avoid
      a dependency towards this library, here is a local substitute.
      Define this substitute only once, even if this file is included
      twice in the same compilation unit.  */
#  ifndef local_wcslen_defined
#   define local_wcslen_defined 1
static size_t
local_wcslen (const wchar_t *s)
{
  const wchar_t *ptr;

  for (ptr = s; *ptr != (wchar_t) 0; ptr++)
    ;
  return ptr - s;
}
#  endif
# endif
#endif

#if WIDE_CHAR_VERSION
# define VASNPRINTF vasnwprintf
# define CHAR_T wchar_t
# define DIRECTIVE wchar_t_directive
# define DIRECTIVES wchar_t_directives
# define PRINTF_PARSE wprintf_parse
# define USE_SNPRINTF 1
# if HAVE_DECL__SNWPRINTF
   /* On Windows, the function swprintf() has a different signature than
      on Unix; we use the _snwprintf() function instead.  */
#  define SNPRINTF _snwprintf
# else
   /* Unix.  */
#  define SNPRINTF swprintf
# endif
#else
# define VASNPRINTF vasnprintf
# define CHAR_T char
# define DIRECTIVE char_directive
# define DIRECTIVES char_directives
# define PRINTF_PARSE printf_parse
# define USE_SNPRINTF (HAVE_DECL__SNPRINTF || HAVE_SNPRINTF)
# if HAVE_DECL__SNPRINTF
   /* Windows.  */
#  define SNPRINTF _snprintf
# else
   /* Unix.  */
#  define SNPRINTF snprintf
# endif
#endif

CHAR_T *
VASNPRINTF (CHAR_T *resultbuf, size_t *lengthp, const CHAR_T *format, va_list args)
{
  DIRECTIVES d;
  arguments a;

  if (PRINTF_PARSE (format, &d, &a) < 0)
    {
      errno = EINVAL;
      return NULL;
    }

#define CLEANUP() \
  free (d.dir);								\
  if (a.arg)								\
    free (a.arg);

  if (printf_fetchargs (args, &a) < 0)
    {
      CLEANUP ();
      errno = EINVAL;
      return NULL;
    }

  {
    size_t buf_neededlength;
    CHAR_T *buf;
    CHAR_T *buf_malloced;
    const CHAR_T *cp;
    size_t i;
    DIRECTIVE *dp;
    /* Output string accumulator.  */
    CHAR_T *result;
    size_t allocated;
    size_t length;

    /* Allocate a small buffer that will hold a directive passed to
       sprintf or snprintf.  */
    buf_neededlength = 7 + d.max_width_length + d.max_precision_length + 6;
#if HAVE_ALLOCA
    if (buf_neededlength < 4000 / sizeof (CHAR_T))
      {
	buf = (CHAR_T *) alloca (buf_neededlength * sizeof (CHAR_T));
	buf_malloced = NULL;
      }
    else
#endif
      {
	if (SIZE_MAX / sizeof (CHAR_T) < buf_neededlength)
	  goto out_of_memory_1;
	buf = (CHAR_T *) malloc (buf_neededlength * sizeof (CHAR_T));
	if (buf == NULL)
	  goto out_of_memory_1;
	buf_malloced = buf;
      }

    if (resultbuf != NULL)
      {
	result = resultbuf;
	allocated = *lengthp;
      }
    else
      {
	result = NULL;
	allocated = 0;
      }
    length = 0;
    /* Invariants:
       result is either == resultbuf or == NULL or malloc-allocated.
       If length > 0, then result != NULL.  */

    /* Ensures that allocated >= length + extra.  Aborts through a jump to
       out_of_memory if size is too big.  */
#define ENSURE_ALLOCATION(extra) \
  {									     \
    size_t needed = length + (extra);					     \
    if (needed < length)						     \
      goto out_of_memory;						     \
    if (needed > allocated)						     \
      {									     \
	size_t memory_size;						     \
	CHAR_T *memory;							     \
									     \
	allocated = (allocated > 0 ? 2 * allocated : 12);		     \
	if (needed > allocated)						     \
	  allocated = needed;						     \
	if (SIZE_MAX / sizeof (CHAR_T) < allocated)			     \
	  goto out_of_memory;						     \
	memory_size = allocated * sizeof (CHAR_T);			     \
	if (result == resultbuf || result == NULL)			     \
	  memory = (CHAR_T *) malloc (memory_size);			     \
	else								     \
	  memory = (CHAR_T *) realloc (result, memory_size);		     \
	if (memory == NULL)						     \
	  goto out_of_memory;						     \
	if (result == resultbuf && length > 0)				     \
	  memcpy (memory, result, length * sizeof (CHAR_T));		     \
	result = memory;						     \
      }									     \
  }

    for (cp = format, i = 0, dp = &d.dir[0]; ; cp = dp->dir_end, i++, dp++)
      {
	if (cp != dp->dir_start)
	  {
	    size_t n = dp->dir_start - cp;

	    ENSURE_ALLOCATION (n);
	    memcpy (result + length, cp, n * sizeof (CHAR_T));
	    length += n;
	  }
	if (i == d.count)
	  break;

	/* Execute a single directive.  */
	if (dp->conversion == '%')
	  {
	    if (!(dp->arg_index == ARG_NONE))
	      abort ();
	    ENSURE_ALLOCATION (1);
	    result[length] = '%';
	    length += 1;
	  }
	else
	  {
	    if (!(dp->arg_index != ARG_NONE))
	      abort ();

	    if (dp->conversion == 'n')
	      {
		switch (a.arg[dp->arg_index].type)
		  {
		  case TYPE_COUNT_SCHAR_POINTER:
		    *a.arg[dp->arg_index].a.a_count_schar_pointer = length;
		    break;
		  case TYPE_COUNT_SHORT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_short_pointer = length;
		    break;
		  case TYPE_COUNT_INT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_int_pointer = length;
		    break;
		  case TYPE_COUNT_LONGINT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_longint_pointer = length;
		    break;
#ifdef HAVE_LONG_LONG
		  case TYPE_COUNT_LONGLONGINT_POINTER:
		    *a.arg[dp->arg_index].a.a_count_longlongint_pointer = length;
		    break;
#endif
		  default:
		    abort ();
		  }
	      }
	    else
	      {
		arg_type type = a.arg[dp->arg_index].type;
		CHAR_T *p;
		unsigned int prefix_count;
		int prefixes[2];
#if !USE_SNPRINTF
		size_t tmp_length;
		CHAR_T tmpbuf[700];
		CHAR_T *tmp;

		/* Allocate a temporary buffer of sufficient size for calling
		   sprintf.  */
		{
		  size_t width;
		  size_t precision;

		  width = 0;
		  if (dp->width_start != dp->width_end)
		    {
		      if (dp->width_arg_index != ARG_NONE)
			{
			  int arg;

			  if (!(a.arg[dp->width_arg_index].type == TYPE_INT))
			    abort ();
			  arg = a.arg[dp->width_arg_index].a.a_int;
			  width = (arg < 0 ? (unsigned int) (-arg) : arg);
			}
		      else
			{
			  const CHAR_T *digitp = dp->width_start;

			  do
			    {
			      size_t w_tmp = width * 10 + (*digitp++ - '0');
			      if (SIZE_MAX / 10 < width || w_tmp < width)
				goto out_of_memory;
			      width = w_tmp;
			    }
			  while (digitp != dp->width_end);
			}
		    }

		  precision = 6;
		  if (dp->precision_start != dp->precision_end)
		    {
		      if (dp->precision_arg_index != ARG_NONE)
			{
			  int arg;

			  if (!(a.arg[dp->precision_arg_index].type == TYPE_INT))
			    abort ();
			  arg = a.arg[dp->precision_arg_index].a.a_int;
			  precision = (arg < 0 ? 0 : arg);
			}
		      else
			{
			  const CHAR_T *digitp = dp->precision_start + 1;

			  precision = 0;
			  while (digitp != dp->precision_end)
			    {
			      size_t p1 = 10 * precision + (*digitp++ - '0');
			      precision = ((SIZE_MAX / 10 < precision
					    || p1 < precision)
					   ? SIZE_MAX : p1);
			    }
			}
		    }

		  switch (dp->conversion)
		    {

		    case 'd': case 'i': case 'u':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      break;

		    case 'o':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.333334 /* binary -> octal */
					 )
			  + 1 /* turn floor into ceil */
			  + 1; /* account for leading sign */
		      break;

		    case 'x': case 'X':
# ifdef HAVE_LONG_LONG
		      if (type == TYPE_LONGLONGINT || type == TYPE_ULONGLONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long long) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      else
# endif
		      if (type == TYPE_LONGINT || type == TYPE_ULONGINT)
			tmp_length =
			  (unsigned int) (sizeof (unsigned long) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      else
			tmp_length =
			  (unsigned int) (sizeof (unsigned int) * CHAR_BIT
					  * 0.25 /* binary -> hexadecimal */
					 )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading sign or alternate form */
		      break;

		    case 'f': case 'F':
# ifdef HAVE_LONG_DOUBLE
		      if (type == TYPE_LONGDOUBLE)
			tmp_length =
			  (unsigned int) (LDBL_MAX_EXP
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 10; /* sign, decimal point etc. */
		      else
# endif
			tmp_length =
			  (unsigned int) (DBL_MAX_EXP
					  * 0.30103 /* binary -> decimal */
					  * 2 /* estimate for FLAG_GROUP */
					 )
			  + 1 /* turn floor into ceil */
			  + 10; /* sign, decimal point etc. */
		      tmp_length += precision;
		      if (tmp_length < precision)
			goto out_of_memory;
		      break;

		    case 'e': case 'E': case 'g': case 'G':
		    case 'a': case 'A':
		      tmp_length =
			12; /* sign, decimal point, exponent etc. */
		      tmp_length += precision;
		      if (tmp_length < precision)
			goto out_of_memory;
		      break;

		    case 'c':
# if defined HAVE_WINT_T && !WIDE_CHAR_VERSION
		      if (type == TYPE_WIDE_CHAR)
			tmp_length = MB_CUR_MAX;
		      else
# endif
			tmp_length = 1;
		      break;

		    case 's':
# ifdef HAVE_WCHAR_T
		      if (type == TYPE_WIDE_STRING)
			{
			  tmp_length =
			    local_wcslen (a.arg[dp->arg_index].a.a_wide_string);

#  if !WIDE_CHAR_VERSION
			  if (SIZE_MAX / MB_CUR_MAX < tmp_length)
			    goto out_of_memory;
			  tmp_length *= MB_CUR_MAX;
#  endif
			}
		      else
# endif
			tmp_length = strlen (a.arg[dp->arg_index].a.a_string);
		      break;

		    case 'p':
		      tmp_length =
			(unsigned int) (sizeof (void *) * CHAR_BIT
					* 0.25 /* binary -> hexadecimal */
				       )
			  + 1 /* turn floor into ceil */
			  + 2; /* account for leading 0x */
		      break;

		    default:
		      abort ();
		    }

		  if (tmp_length < width)
		    tmp_length = width;

		  tmp_length++; /* account for trailing NUL */
		  if (!tmp_length)
		    goto out_of_memory;
		}

		if (tmp_length <= sizeof (tmpbuf) / sizeof (CHAR_T))
		  tmp = tmpbuf;
		else
		  {
		    if (SIZE_MAX / sizeof (CHAR_T) < tmp_length)
		      /* Overflow, would lead to out of memory.  */
		      goto out_of_memory;
		    tmp = (CHAR_T *) malloc (tmp_length * sizeof (CHAR_T));
		    if (tmp == NULL)
		      /* Out of memory.  */
		      goto out_of_memory;
		  }
#endif

		/* Construct the format string for calling snprintf or
		   sprintf.  */
		p = buf;
		*p++ = '%';
		if (dp->flags & FLAG_GROUP)
		  *p++ = '\'';
		if (dp->flags & FLAG_LEFT)
		  *p++ = '-';
		if (dp->flags & FLAG_SHOWSIGN)
		  *p++ = '+';
		if (dp->flags & FLAG_SPACE)
		  *p++ = ' ';
		if (dp->flags & FLAG_ALT)
		  *p++ = '#';
		if (dp->flags & FLAG_ZERO)
		  *p++ = '0';
		if (dp->width_start != dp->width_end)
		  {
		    size_t n = dp->width_end - dp->width_start;
		    memcpy (p, dp->width_start, n * sizeof (CHAR_T));
		    p += n;
		  }
		if (dp->precision_start != dp->precision_end)
		  {
		    size_t n = dp->precision_end - dp->precision_start;
		    memcpy (p, dp->precision_start, n * sizeof (CHAR_T));
		    p += n;
		  }

		switch (type)
		  {
#ifdef HAVE_LONG_LONG
		  case TYPE_LONGLONGINT:
		  case TYPE_ULONGLONGINT:
		    *p++ = 'l';
		    /*FALLTHROUGH*/
#endif
		  case TYPE_LONGINT:
		  case TYPE_ULONGINT:
#ifdef HAVE_WINT_T
		  case TYPE_WIDE_CHAR:
#endif
#ifdef HAVE_WCHAR_T
		  case TYPE_WIDE_STRING:
#endif
		    *p++ = 'l';
		    break;
#ifdef HAVE_LONG_DOUBLE
		  case TYPE_LONGDOUBLE:
		    *p++ = 'L';
		    break;
#endif
		  default:
		    break;
		  }
		*p = dp->conversion;
#if USE_SNPRINTF
		p[1] = '%';
		p[2] = 'n';
		p[3] = '\0';
#else
		p[1] = '\0';
#endif

		/* Construct the arguments for calling snprintf or sprintf.  */
		prefix_count = 0;
		if (dp->width_arg_index != ARG_NONE)
		  {
		    if (!(a.arg[dp->width_arg_index].type == TYPE_INT))
		      abort ();
		    prefixes[prefix_count++] = a.arg[dp->width_arg_index].a.a_int;
		  }
		if (dp->precision_arg_index != ARG_NONE)
		  {
		    if (!(a.arg[dp->precision_arg_index].type == TYPE_INT))
		      abort ();
		    prefixes[prefix_count++] = a.arg[dp->precision_arg_index].a.a_int;
		  }

#if USE_SNPRINTF
		/* Prepare checking whether snprintf returns the count
		   via %n.  */
		ENSURE_ALLOCATION (1);
		result[length] = '\0';
#endif

		for (;;)
		  {
		    size_t maxlen;
		    int count;
		    int retcount;

		    maxlen = allocated - length;
		    count = -1;
		    retcount = 0;

#if USE_SNPRINTF
# define SNPRINTF_BUF(arg) \
		    switch (prefix_count)				    \
		      {							    \
		      case 0:						    \
			retcount = SNPRINTF (result + length, maxlen, buf,  \
					     arg, &count);		    \
			break;						    \
		      case 1:						    \
			retcount = SNPRINTF (result + length, maxlen, buf,  \
					     prefixes[0], arg, &count);	    \
			break;						    \
		      case 2:						    \
			retcount = SNPRINTF (result + length, maxlen, buf,  \
					     prefixes[0], prefixes[1], arg, \
					     &count);			    \
			break;						    \
		      default:						    \
			abort ();					    \
		      }
#else
# define SNPRINTF_BUF(arg) \
		    switch (prefix_count)				    \
		      {							    \
		      case 0:						    \
			count = sprintf (tmp, buf, arg);		    \
			break;						    \
		      case 1:						    \
			count = sprintf (tmp, buf, prefixes[0], arg);	    \
			break;						    \
		      case 2:						    \
			count = sprintf (tmp, buf, prefixes[0], prefixes[1],\
					 arg);				    \
			break;						    \
		      default:						    \
			abort ();					    \
		      }
#endif

		    switch (type)
		      {
		      case TYPE_SCHAR:
			{
			  int arg = a.arg[dp->arg_index].a.a_schar;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_UCHAR:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_uchar;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_SHORT:
			{
			  int arg = a.arg[dp->arg_index].a.a_short;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_USHORT:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_ushort;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_INT:
			{
			  int arg = a.arg[dp->arg_index].a.a_int;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_UINT:
			{
			  unsigned int arg = a.arg[dp->arg_index].a.a_uint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_LONGINT:
			{
			  long int arg = a.arg[dp->arg_index].a.a_longint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_ULONGINT:
			{
			  unsigned long int arg = a.arg[dp->arg_index].a.a_ulongint;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_LONG_LONG
		      case TYPE_LONGLONGINT:
			{
			  long long int arg = a.arg[dp->arg_index].a.a_longlongint;
			  SNPRINTF_BUF (arg);
			}
			break;
		      case TYPE_ULONGLONGINT:
			{
			  unsigned long long int arg = a.arg[dp->arg_index].a.a_ulonglongint;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_DOUBLE:
			{
			  double arg = a.arg[dp->arg_index].a.a_double;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_LONG_DOUBLE
		      case TYPE_LONGDOUBLE:
			{
			  long double arg = a.arg[dp->arg_index].a.a_longdouble;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_CHAR:
			{
			  int arg = a.arg[dp->arg_index].a.a_char;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_WINT_T
		      case TYPE_WIDE_CHAR:
			{
			  wint_t arg = a.arg[dp->arg_index].a.a_wide_char;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_STRING:
			{
			  const char *arg = a.arg[dp->arg_index].a.a_string;
			  SNPRINTF_BUF (arg);
			}
			break;
#ifdef HAVE_WCHAR_T
		      case TYPE_WIDE_STRING:
			{
			  const wchar_t *arg = a.arg[dp->arg_index].a.a_wide_string;
			  SNPRINTF_BUF (arg);
			}
			break;
#endif
		      case TYPE_POINTER:
			{
			  void *arg = a.arg[dp->arg_index].a.a_pointer;
			  SNPRINTF_BUF (arg);
			}
			break;
		      default:
			abort ();
		      }

#if USE_SNPRINTF
		    /* Portability: Not all implementations of snprintf()
		       are ISO C 99 compliant.  Determine the number of
		       bytes that snprintf() has produced or would have
		       produced.  */
		    if (count >= 0)
		      {
			/* Verify that snprintf() has NUL-terminated its
			   result.  */
			if (count < maxlen && result[length + count] != '\0')
			  abort ();
			/* Portability hack.  */
			if (retcount > count)
			  count = retcount;
		      }
		    else
		      {
			/* snprintf() doesn't understand the '%n'
			   directive.  */
			if (p[1] != '\0')
			  {
			    /* Don't use the '%n' directive; instead, look
			       at the snprintf() return value.  */
			    p[1] = '\0';
			    continue;
			  }
			else
			  {
			    /* Look at the snprintf() return value.  */
			    if (retcount < 0)
			      {
				/* HP-UX 10.20 snprintf() is doubly deficient:
				   It doesn't understand the '%n' directive,
				   *and* it returns -1 (rather than the length
				   that would have been required) when the
				   buffer is too small.  */
				size_t bigger_need =
				  (allocated > 12 ? allocated : 12);
				ENSURE_ALLOCATION (bigger_need);
				continue;
			      }
			    else
			      count = retcount;
			  }
		      }
#endif

		    /* Attempt to handle failure.  */
		    if (count < 0)
		      {
			if (!(result == resultbuf || result == NULL))
			  free (result);
			if (buf_malloced != NULL)
			  free (buf_malloced);
			CLEANUP ();
			errno = EINVAL;
			return NULL;
		      }

#if !USE_SNPRINTF
		    if (count >= tmp_length)
		      /* tmp_length was incorrectly calculated - fix the
			 code above!  */
		      abort ();
#endif

		    /* Make room for the result.  */
		    if (count >= maxlen)
		      {
			/* Need at least count bytes.  But allocate
			   proportionally, to avoid looping eternally if
			   snprintf() reports a too small count.  */
			ENSURE_ALLOCATION (count < allocated
					   ? allocated : count);
#if USE_SNPRINTF
			continue;
#endif
		      }

#if USE_SNPRINTF
		    /* The snprintf() result did fit.  */
#else
		    /* Append the sprintf() result.  */
		    memcpy (result + length, tmp, count * sizeof (CHAR_T));
		    if (tmp != tmpbuf)
		      free (tmp);
#endif

		    length += count;
		    break;
		  }
	      }
	  }
      }

    /* Add the final NUL.  */
    ENSURE_ALLOCATION (1);
    result[length] = '\0';

    if (result != resultbuf && length + 1 < allocated)
      {
	/* Shrink the allocated memory if possible.  */
	CHAR_T *memory;

	memory = (CHAR_T *) realloc (result, (length + 1) * sizeof (CHAR_T));
	if (memory != NULL)
	  result = memory;
      }

    if (buf_malloced != NULL)
      free (buf_malloced);
    CLEANUP ();
    *lengthp = length;
    if (length > INT_MAX)
      goto length_overflow;
    return result;

  length_overflow:
    /* We could produce such a big string, but its length doesn't fit into
       an 'int'.  POSIX says that snprintf() fails with errno = EOVERFLOW in
       this case.  */
    if (result != resultbuf)
      free (result);
    errno = EOVERFLOW;
    return NULL;

  out_of_memory:
    if (!(result == resultbuf || result == NULL))
      free (result);
    if (buf_malloced != NULL)
      free (buf_malloced);
  out_of_memory_1:
    CLEANUP ();
    errno = ENOMEM;
    return NULL;
  }
}

#undef SNPRINTF
#undef USE_SNPRINTF
#undef PRINTF_PARSE
#undef DIRECTIVES
#undef DIRECTIVE
#undef CHAR_T
#undef VASNPRINTF
