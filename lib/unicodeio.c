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
#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if HAVE_ICONV
# include <iconv.h>
#endif

#include <error.h>

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif

#include "unicodeio.h"

/* When we pass a Unicode character to iconv(), we must pass it in a
   suitable encoding. The standardized Unicode encodings are
   UTF-8, UCS-2, UCS-4, UTF-16, UTF-16BE, UTF-16LE, UTF-7.
   UCS-2 supports only characters up to \U0000FFFF.
   UTF-16 and variants support only characters up to \U0010FFFF.
   UTF-7 is way too complex and not supported by glibc-2.1.
   UCS-4 specification leaves doubts about endianness and byte order
   mark. glibc currently interprets it as big endian without byte order
   mark, but this is not backed by an RFC.
   So we use UTF-8. It supports characters up to \U7FFFFFFF and is
   unambiguously defined.  */

/* Stores the UTF-8 representation of the Unicode character wc in r[0..5].
   Returns the number of bytes stored, or -1 if wc is out of range.  */
static int
utf8_wctomb (unsigned char *r, unsigned int wc)
{
  int count;

  if (wc < 0x80)
    count = 1;
  else if (wc < 0x800)
    count = 2;
  else if (wc < 0x10000)
    count = 3;
  else if (wc < 0x200000)
    count = 4;
  else if (wc < 0x4000000)
    count = 5;
  else if (wc <= 0x7fffffff)
    count = 6;
  else
    return -1;

  switch (count)
    {
      /* Note: code falls through cases! */
      case 6: r[5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
      case 5: r[4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
      case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
      case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
      case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
      case 1: r[0] = wc;
    }

  return count;
}

/* Luckily, the encoding's name is platform independent.  */
#define UTF8_NAME "UTF-8"

/* Outputs the Unicode character CODE to the output stream STREAM.
   Assumes that the locale doesn't change between two calls.  */
void
print_unicode_char (FILE *stream, unsigned int code)
{
  static int initialized;
  static int is_utf8;
#if HAVE_ICONV
  static iconv_t utf8_to_local;
#endif

  char inbuf[6];
  int count;

  if (!initialized)
    {
      extern const char *locale_charset PARAMS ((void));
      const char *charset = locale_charset ();

      is_utf8 = (charset != NULL && !strcmp (charset, UTF8_NAME));
#if HAVE_ICONV
      if (!is_utf8)
	{
	  utf8_to_local = (charset != NULL
			   ? iconv_open (charset, UTF8_NAME)
			   : (iconv_t)(-1));
	  if (utf8_to_local == (iconv_t)(-1))
	    {
	      /* For an unknown encoding, assume ASCII.  */
	      utf8_to_local = iconv_open ("ASCII", UTF8_NAME);
	      if (utf8_to_local == (iconv_t)(-1))
		error (1, 0,
		       _("cannot output U+%04X: iconv function not usable"),
		       code);
	    }
	}
#endif
      initialized = 1;
    }

  /* Convert the character to UTF-8.  */
  count = utf8_wctomb ((unsigned char *) inbuf, code);
  if (count < 0)
    error (1, 0, _("U+%04X: character out of range"), code);

  if (is_utf8)
    {
      fwrite (inbuf, 1, count, stream);
    }
  else
    {
#if HAVE_ICONV
      char outbuf[25];
      const char *inptr;
      size_t inbytesleft;
      char *outptr;
      size_t outbytesleft;
      size_t res;

      inptr = inbuf;
      inbytesleft = count;
      outptr = outbuf;
      outbytesleft = sizeof (outbuf);

      /* Convert the character from UTF-8 to the locale's charset.  */
      res = iconv (utf8_to_local,
		   (ICONV_CONST char **)&inptr, &inbytesleft,
		   &outptr, &outbytesleft);
      if (inbytesleft > 0 || res == (size_t)(-1)
	  /* Irix iconv() inserts a NUL byte if it cannot convert. */
# if !defined _LIBICONV_VERSION && (defined sgi || defined __sgi)
	  || (res > 0 && code != 0 && outptr - outbuf == 1 && *outbuf == '\0')
# endif
         )
	error (1, res == (size_t)(-1) ? errno : 0,
	       _("cannot convert U+%04X to local character set"), code);

      /* Avoid glibc-2.1 bug and Solaris 2.7 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)

      /* Get back to the initial shift state.  */
      res = iconv (utf8_to_local, NULL, NULL, &outptr, &outbytesleft);
      if (res == (size_t)(-1))
	error (1, errno, _("cannot convert U+%04X to local character set"),
	       code);
# endif

      fwrite (outbuf, 1, outptr - outbuf, stream);
#else
      error (1, 0, _("cannot output U+%04X: iconv function not available"),
	     code);
#endif
    }
}
