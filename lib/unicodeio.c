/* Unicode character output to streams with locale dependent encoding.

   Copyright (C) 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* Written by Bruno Haible <haible@clisp.cons.org>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STDDEF_H
# include <stddef.h>
#endif

#include <stdio.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if HAVE_ICONV
# include <iconv.h>
/* Name of UCS-4 encoding with machine dependent endianness and alignment.  */
# ifdef _LIBICONV_VERSION
#  define UCS4_NAME "UCS-4-INTERNAL"
# else
#  define UCS4_NAME "INTERNAL"
# endif
#endif

#include <error.h>

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif

#include "unicodeio.h"

/* Use md5.h for its nice detection of unsigned 32-bit type.  */
#include "md5.h"
#undef uint32_t
#define uint32_t md5_uint32

/* Outputs the Unicode character CODE to the output stream STREAM.
   Assumes that the locale doesn't change between two calls.  */
void
print_unicode_char (FILE *stream, unsigned int code)
{
#if HAVE_ICONV
  static int initialized;
  static iconv_t ucs4_to_local;

  uint32_t in;
  char outbuf[25];
  const char *inptr;
  size_t inbytesleft;
  char *outptr;
  size_t outbytesleft;
  size_t res;

  if (!initialized)
    {
      extern const char *locale_charset (void);
      const char *charset = locale_charset ();

      ucs4_to_local = (charset != NULL
		       ? iconv_open (charset, UCS4_NAME)
		       : (iconv_t)(-1));
      if (ucs4_to_local == (iconv_t)(-1))
	{
	  /* For an unknown encoding, assume ASCII.  */
	  ucs4_to_local = iconv_open ("ASCII", UCS4_NAME);
	  if (ucs4_to_local == (iconv_t)(-1))
	    error (1, 0, _("cannot output U+%04X: iconv function not usable"),
		   code);
	}
      initialized = 1;
    }

  in = code;
  inptr = (char *) &in;
  inbytesleft = sizeof (in);
  outptr = outbuf;
  outbytesleft = sizeof (outbuf);

  /* Convert the character.  */
  res = iconv (ucs4_to_local, &inptr, &inbytesleft, &outptr, &outbytesleft);
  if (inbytesleft > 0 || res == (size_t)(-1))
    error (1, res == (size_t)(-1) ? errno : 0,
	   _("cannot convert U+%04X to local character set"), code);

  /* Avoid glibc-2.1 bug.  */
# if defined _LIBICONV_VERSION || !(__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1)

  /* Get back to the initial shift state.  */
  res = iconv (ucs4_to_local, NULL, NULL, &outptr, &outbytesleft);
  if (res == (size_t)(-1))
    error (1, errno, _("cannot convert U+%04X to local character set"), code);

# endif

  fwrite (outbuf, 1, outptr - outbuf, stream);

#else
  error (1, 0, _("cannot output U+%04X: iconv function not available"), code);
#endif
}
