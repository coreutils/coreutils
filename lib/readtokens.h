/* readtokens.h -- Functions for reading tokens from an input stream.

   Copyright (C) 1990, 1991, 1999, 2001, 2003 Free Software Foundation, Inc.

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

#ifndef H_READTOKENS_H
# define H_READTOKENS_H

# include <stdio.h>

# ifndef INITIAL_TOKEN_LENGTH
#  define INITIAL_TOKEN_LENGTH 20
# endif

# ifndef TOKENBUFFER_DEFINED
#  define TOKENBUFFER_DEFINED
struct tokenbuffer
{
  long size;
  char *buffer;
};
typedef struct tokenbuffer token_buffer;

# endif /* not TOKENBUFFER_DEFINED */

void init_tokenbuffer (token_buffer *tokenbuffer);

long
  readtoken (FILE *stream, const char *delim, int n_delim,
	     token_buffer *tokenbuffer);
int
  readtokens (FILE *stream, int projected_n_tokens,
	      const char *delim, int n_delim,
	      char ***tokens_out, long **token_lengths);

#endif /* not H_READTOKENS_H */
