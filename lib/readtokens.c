/* readtokens.c  -- Functions for reading tokens from an input stream.
   Copyright (C) 1990-1991, 1999, 2001, 2003 Jim Meyering.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written by Jim Meyering. */

/* This almost supercedes xreadline stuff -- using delim="\n"
   gives the same functionality, except that these functions
   would never return empty lines.

   To Do:
     - To allow '\0' as a delimiter, I will have to change
       interfaces to permit specification of delimiter-string
       length.
   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#if defined (STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
# if !defined (STDC_HEADERS) && defined (HAVE_MEMORY_H)
#  include <memory.h>
# endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
# include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#include "readtokens.h"
#include "unlocked-io.h"
#include "xalloc.h"

#define STREQ(a,b) ((a) == (b) || ((a) && (b) && *(a) == *(b) \
				   && strcmp(a, b) == 0))

/* Initialize a tokenbuffer. */

void
init_tokenbuffer (tokenbuffer)
     token_buffer *tokenbuffer;
{
  tokenbuffer->size = INITIAL_TOKEN_LENGTH;
  tokenbuffer->buffer = ((char *) xmalloc (INITIAL_TOKEN_LENGTH));
}

/* Read a token from `stream' into `tokenbuffer'.
   Upon return, the token is in tokenbuffer->buffer and
   has a trailing '\0' instead of the original delimiter.
   The function value is the length of the token not including
   the final '\0'.  When EOF is reached (i.e. on the call
   after the last token is read), -1 is returned and tokenbuffer
   isn't modified.

   This function will work properly on lines containing NUL bytes
   and on files that aren't newline-terminated.  */

long
readtoken (FILE *stream,
	   const char *delim,
	   int n_delim,
	   token_buffer *tokenbuffer)
{
  char *p;
  int c, i, n;
  static const char *saved_delim = NULL;
  static char isdelim[256];
  int same_delimiters;

  if (delim == NULL && saved_delim == NULL)
    abort ();

  same_delimiters = 0;
  if (delim != saved_delim && saved_delim != NULL)
    {
      same_delimiters = 1;
      for (i = 0; i < n_delim; i++)
	{
	  if (delim[i] != saved_delim[i])
	    {
	      same_delimiters = 0;
	      break;
	    }
	}
    }

  if (!same_delimiters)
    {
      const char *t;
      unsigned int j;
      saved_delim = delim;
      for (j = 0; j < sizeof (isdelim); j++)
	isdelim[j] = 0;
      for (t = delim; *t; t++)
	isdelim[(unsigned char) *t] = 1;
    }

  p = tokenbuffer->buffer;
  n = tokenbuffer->size;
  i = 0;

  /* FIXME: don't fool with this caching BS.  Use strchr instead.  */
  /* skip over any leading delimiters */
  for (c = getc (stream); c >= 0 && isdelim[c]; c = getc (stream))
    {
      /* empty */
    }

  for (;;)
    {
      if (i >= n)
	{
	  n = 3 * (n / 2 + 1);
	  p = xrealloc (p, (unsigned int) n);
	}
      if (c < 0)
	{
	  if (i == 0)
	    return (-1);
	  p[i] = 0;
	  break;
	}
      if (isdelim[c])
	{
	  p[i] = 0;
	  break;
	}
      p[i++] = c;
      c = getc (stream);
    }

  tokenbuffer->buffer = p;
  tokenbuffer->size = n;
  return (i);
}

/* Return a NULL-terminated array of pointers to tokens
   read from `stream.'  The number of tokens is returned
   as the value of the function.
   All storage is obtained through calls to malloc();

   %%% Question: is it worth it to do a single
   %%% realloc() of `tokens' just before returning? */

int
readtokens (FILE *stream,
	    int projected_n_tokens,
	    const char *delim,
	    int n_delim,
	    char ***tokens_out,
	    long **token_lengths)
{
  token_buffer tb, *token = &tb;
  int token_length;
  char **tokens;
  long *lengths;
  int sz;
  int n_tokens;

  n_tokens = 0;
  if (projected_n_tokens > 0)
    projected_n_tokens++;	/* add one for trailing NULL pointer */
  else
    projected_n_tokens = 64;
  sz = projected_n_tokens;
  tokens = (char **) xmalloc (sz * sizeof (char *));
  lengths = (long *) xmalloc (sz * sizeof (long));

  init_tokenbuffer (token);
  for (;;)
    {
      char *tmp;
      token_length = readtoken (stream, delim, n_delim, token);
      if (n_tokens >= sz)
	{
	  sz *= 2;
	  tokens = (char **) xrealloc (tokens, sz * sizeof (char *));
	  lengths = (long *) xrealloc (lengths, sz * sizeof (long));
	}

      if (token_length < 0)
	{
	  /* don't increment n_tokens for NULL entry */
	  tokens[n_tokens] = NULL;
	  lengths[n_tokens] = -1;
	  break;
	}
      tmp = (char *) xmalloc ((token_length + 1) * sizeof (char));
      lengths[n_tokens] = token_length;
      tokens[n_tokens] = strncpy (tmp, token->buffer,
				  (unsigned) (token_length + 1));
      n_tokens++;
    }

  free (token->buffer);
  *tokens_out = tokens;
  if (token_lengths != NULL)
    *token_lengths = lengths;
  return n_tokens;
}
