/* quotearg.c - quote arguments for output
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.

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

/* Written by Paul Eggert <eggert@twinsun.com> */

/* FIXME: Multibyte characters are not supported yet.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <quotearg.h>
#include <xalloc.h>

#include <ctype.h>
#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define ISASCII(c) 1
#else
# define ISASCII(c) isascii (c)
#endif
#ifdef isgraph
# define ISGRAPH(c) (ISASCII (c) && isgraph (c))
#else
# define ISGRAPH(c) (ISASCII (c) && isprint (c) && !isspace (c))
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(text) gettext (text)
#else
# define _(text) text
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif
#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif
#ifndef UCHAR_MAX
# define UCHAR_MAX ((unsigned char) -1)
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#define INT_BITS (sizeof (int) * CHAR_BIT)

struct quoting_options
{
  /* Basic quoting style.  */
  enum quoting_style style;

  /* Quote the chararacters indicated by this bit vector even if the
     quoting style would not normally require them to be quoted.  */
  int quote_these_too[((UCHAR_MAX + 1) / INT_BITS
		       + ((UCHAR_MAX + 1) % INT_BITS != 0))];
};

/* Names of quoting styles.  */
char const *const quoting_style_args[] =
{
  "literal",
  "shell",
  "shell-always",
  "c",
  "escape",
  "locale",
  0
};

/* Correspondances to quoting style names.  */
enum quoting_style const quoting_style_vals[] =
{
  literal_quoting_style,
  shell_quoting_style,
  shell_always_quoting_style,
  c_quoting_style,
  escape_quoting_style,
  locale_quoting_style
};

/* The default quoting options.  */
static struct quoting_options default_quoting_options;

/* Allocate a new set of quoting options, with contents initially identical
   to O if O is not null, or to the default if O is null.
   It is the caller's responsibility to free the result.  */
struct quoting_options *
clone_quoting_options (struct quoting_options *o)
{
  struct quoting_options *p
    = (struct quoting_options *) xmalloc (sizeof (struct quoting_options));
  *p = *(o ? o : &default_quoting_options);
  return p;
}

/* Get the value of O's quoting style.  If O is null, use the default.  */
enum quoting_style
get_quoting_style (struct quoting_options *o)
{
  return (o ? o : &default_quoting_options)->style;
}

/* In O (or in the default if O is null),
   set the value of the quoting style to S.  */
void
set_quoting_style (struct quoting_options *o, enum quoting_style s)
{
  (o ? o : &default_quoting_options)->style = s;
}

/* In O (or in the default if O is null),
   set the value of the quoting options for character C to I.
   Return the old value.  Currently, the only values defined for I are
   0 (the default) and 1 (which means to quote the character even if
   it would not otherwise be quoted).  */
int
set_char_quoting (struct quoting_options *o, char c, int i)
{
  unsigned char uc = c;
  int *p = (o ? o : &default_quoting_options)->quote_these_too + uc / INT_BITS;
  int shift = uc % INT_BITS;
  int r = (*p >> shift) & 1;
  *p ^= ((i & 1) ^ r) << shift;
  return r;
}

/* Place into buffer BUFFER (of size BUFFERSIZE) a quoted version of
   argument ARG (of size ARGSIZE), using O to control quoting.
   If O is null, use the default.
   Terminate the output with a null character, and return the written
   size of the output, not counting the terminating null.
   If BUFFERSIZE is too small to store the output string, return the
   value that would have been returned had BUFFERSIZE been large enough.
   If ARGSIZE is -1, use the string length of the argument for ARGSIZE.  */
size_t
quotearg_buffer (char *buffer, size_t buffersize,
		 char const *arg, size_t argsize,
		 struct quoting_options const *o)
{
  unsigned char c;
  size_t i;
  size_t len = 0;
  char const *quote_string;
  size_t quote_string_len;
  struct quoting_options const *p = o ? o : &default_quoting_options;
  enum quoting_style quoting_style = p->style;
#define STORE(c) \
    do \
      { \
	if (len < buffersize) \
	  buffer[len] = (c); \
	  len++; \
      } \
    while (0)

  switch (quoting_style)
    {
    case shell_quoting_style:
      if (! (argsize == (size_t) -1 ? arg[0] == '\0' : argsize == 0))
	{
	  switch (arg[0])
	    {
	    case '#': case '~':
	      break;

	    default:
	      for (i = 0; ; i++)
		{
		  if (argsize == (size_t) -1 ? arg[i] == '\0' : i == argsize)
		    goto done;

		  c = arg[i];

		  switch (c)
		    {
		    case '\t': case '\n': case ' ':
		    case '!': /* special in csh */
		    case '"': case '$': case '&': case '\'':
		    case '(': case ')': case '*': case ';':
		    case '<': case '>': case '?': case '[': case '\\':
		    case '^': /* special in old /bin/sh, e.g. SunOS 4.1.4 */
		    case '`': case '|':
		      goto needs_quoting;
		    }

		  if (p->quote_these_too[c / INT_BITS] & (1 << (c % INT_BITS)))
		    goto needs_quoting;

		  STORE (c);
		}
	    needs_quoting:;

	      len = 0;
	      break;
	    }
	}
      /* Fall through.  */

    case shell_always_quoting_style:
      STORE ('\'');
      quote_string = "'";
      quote_string_len = 1;
      break;

    case c_quoting_style:
      STORE ('"');
      quote_string = "\"";
      quote_string_len = 1;
      break;

    case locale_quoting_style:
      for (quote_string = _("`"); *quote_string; quote_string++)
	STORE (*quote_string);
      quote_string = _("'");
      quote_string_len = strlen (quote_string);
      break;

    default:
      quote_string = 0;
      quote_string_len = 0;
      break;
    }

  for (i = 0;  ! (argsize == (size_t) -1 ? arg[i] == '\0' : i == argsize);  i++)
    {
      c = arg[i];

      switch (quoting_style)
	{
	case literal_quoting_style:
	  break;

	case shell_quoting_style:
	case shell_always_quoting_style:
	  if (c == '\'')
	    {
	      STORE ('\'');
	      STORE ('\\');
	      STORE ('\'');
	    }
	  break;

	case c_quoting_style:
	case escape_quoting_style:
	case locale_quoting_style:
	  switch (c)
	    {
	    case '?': /* Do not generate trigraphs.  */
	    case '\\': goto store_escape;
	      /* Not all C compilers know what \a means.  */
	    case   7 : c = 'a'; goto store_escape;
	    case '\b': c = 'b'; goto store_escape;
	    case '\f': c = 'f'; goto store_escape;
	    case '\n': c = 'n'; goto store_escape;
	    case '\r': c = 'r'; goto store_escape;
	    case '\t': c = 't'; goto store_escape;
	    case '\v': c = 'v'; goto store_escape;

	    case ' ': break;

	    default:
	      if (quote_string_len
		  && strncmp (arg + i, quote_string, quote_string_len) == 0)
		goto store_escape;
	      if (!ISGRAPH (c))
		{
		  STORE ('\\');
		  STORE ('0' + (c >> 6));
		  STORE ('0' + ((c >> 3) & 7));
		  c = '0' + (c & 7);
		  goto store_c;
		}
	      break;
	    }

	  if (! (p->quote_these_too[c / INT_BITS] & (1 << (c % INT_BITS))))
	    goto store_c;

	store_escape:
	  STORE ('\\');
	}

    store_c:
      STORE (c);
    }

  if (quote_string)
    for (; *quote_string; quote_string++)
      STORE (*quote_string);

 done:
  if (len < buffersize)
    buffer[len] = '\0';
  return len;
}

/* Use storage slot N to return a quoted version of the string ARG.
   OPTIONS specifies the quoting options.
   The returned value points to static storage that can be
   reused by the next call to this function with the same value of N.
   N must be nonnegative.  N is deliberately declared with type `int'
   to allow for future extensions (using negative values).  */
static char *
quotearg_n_options (int n, char const *arg,
		    struct quoting_options const *options)
{
  static unsigned int nslots;
  static struct slotvec
    {
      size_t size;
      char *val;
    } *slotvec;

  if (nslots <= n)
    {
      int n1 = n + 1;
      size_t s = n1 * sizeof (struct slotvec);
      if (! (0 < n1 && n1 == s / sizeof (struct slotvec)))
	abort ();
      slotvec = (struct slotvec *) xrealloc (slotvec, s);
      memset (slotvec + nslots, 0, (n1 - nslots) * sizeof (struct slotvec));
      nslots = n;
    }

  {
    size_t size = slotvec[n].size;
    char *val = slotvec[n].val;
    size_t qsize = quotearg_buffer (val, size, arg, (size_t) -1, options);

    if (size <= qsize)
      {
	slotvec[n].size = size = qsize + 1;
	slotvec[n].val = val = xrealloc (val, size);
	quotearg_buffer (val, size, arg, (size_t) -1, options);
      }

    return val;
  }
}

char *
quotearg_n (unsigned int n, char const *arg)
{
  return quotearg_n_options (n, arg, &default_quoting_options);
}

char *
quotearg (char const *arg)
{
  return quotearg_n (0, arg);
}

char *
quotearg_n_style (unsigned int n, enum quoting_style s, char const *arg)
{
  struct quoting_options o;
  o.style = s;
  memset (o.quote_these_too, 0, sizeof o.quote_these_too);
  return quotearg_n_options (n, arg, &o);
}

char *
quotearg_style (enum quoting_style s, char const *arg)
{
  return quotearg_n_style (0, s, arg);
}

char *
quotearg_char (char const *arg, char ch)
{
  struct quoting_options options;
  options = default_quoting_options;
  set_char_quoting (&options, ch, 1);
  return quotearg_n_options (0, arg, &options);
}

char *
quotearg_colon (char const *arg)
{
  return quotearg_char (arg, ':');
}
