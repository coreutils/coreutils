/* Unicode character output to streams with locale dependent encoding.

   Copyright (C) 2000-2002 Free Software Foundation, Inc.

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

/* Note: This file requires the locale_charset() function.  See in
   libiconv-1.8/libcharset/INTEGRATE for how to obtain it.  */

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

#include "gettext.h"
#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

/* Specification.  */
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

/* Converts the Unicode character CODE to its multibyte representation
   in the current locale and calls the SUCCESS callback on the resulting
   byte sequence.  If an error occurs, invokes the FAILURE callback instead,
   passing it CODE and an English error string.
   Returns whatever the callback returned.
   Assumes that the locale doesn't change between two calls.  */
long
unicode_to_mb (unsigned int code,
	       long (*success) PARAMS ((const char *buf, size_t buflen,
					void *callback_arg)),
	       long (*failure) PARAMS ((unsigned int code, const char *msg,
					void *callback_arg)),
	       void *callback_arg)
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

      is_utf8 = !strcmp (charset, UTF8_NAME);
#if HAVE_ICONV
      if (!is_utf8)
	{
	  utf8_to_local = iconv_open (charset, UTF8_NAME);
	  if (utf8_to_local == (iconv_t)(-1))
	    /* For an unknown encoding, assume ASCII.  */
	    utf8_to_local = iconv_open ("ASCII", UTF8_NAME);
	}
#endif
      initialized = 1;
    }

  /* Test whether the utf8_to_local converter is available at all.  */
  if (!is_utf8)
    {
#if HAVE_ICONV
      if (utf8_to_local == (iconv_t)(-1))
	return failure (code, N_("iconv function not usable"), callback_arg);
#else
      return failure (code, N_("iconv function not available"), callback_arg);
#endif
    }

  /* Convert the character to UTF-8.  */
  count = utf8_wctomb ((unsigned char *) inbuf, code);
  if (count < 0)
    return failure (code, N_("character out of range"), callback_arg);

#if HAVE_ICONV
  if (!is_utf8)
    {
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
	return failure (code, NULL, callback_arg);

      /* Avoid glibc-2.1 bug and Solaris 2.7 bug.  */
# if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)

      /* Get back to the initial shift state.  */
      res = iconv (utf8_to_local, NULL, NULL, &outptr, &outbytesleft);
      if (res == (size_t)(-1))
	return failure (code, NULL, callback_arg);
# endif

      return success (outbuf, outptr - outbuf, callback_arg);
    }
#endif

  /* At this point, is_utf8 is true, so no conversion is needed.  */
  return success (inbuf, count, callback_arg);
}

/* Simple success callback that outputs the converted string.
   The STREAM is passed as callback_arg.  */
long
fwrite_success_callback (const char *buf, size_t buflen, void *callback_arg)
{
  FILE *stream = (FILE *) callback_arg;

  fwrite (buf, 1, buflen, stream);
  return 0;
}

/* Simple failure callback that displays an error and exits.  */
static long
exit_failure_callback (unsigned int code, const char *msg, void *callback_arg)
{
  if (msg == NULL)
    error (1, 0, _("cannot convert U+%04X to local character set"), code);
  else
    error (1, 0, _("cannot convert U+%04X to local character set: %s"), code,
	   gettext (msg));
  return -1;
}

/* Simple failure callback that displays a fallback representation in plain
   ASCII, using the same notation as ISO C99 strings.  */
static long
fallback_failure_callback (unsigned int code, const char *msg, void *callback_arg)
{
  FILE *stream = (FILE *) callback_arg;

  if (code < 0x10000)
    fprintf (stream, "\\u%04X", code);
  else
    fprintf (stream, "\\U%08X", code);
  return -1;
}

/* Outputs the Unicode character CODE to the output stream STREAM.
   Upon failure, exit if exit_on_error is true, otherwise output a fallback
   notation.  */
void
print_unicode_char (FILE *stream, unsigned int code, int exit_on_error)
{
  unicode_to_mb (code, fwrite_success_callback,
		 exit_on_error
		 ? exit_failure_callback
		 : fallback_failure_callback,
		 stream);
}
