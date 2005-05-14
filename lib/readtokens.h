/* readtokens.h -- Functions for reading tokens from an input stream.

   Copyright (C) 1990, 1991, 1999, 2001-2004 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Jim Meyering. */

#ifndef READTOKENS_H
# define READTOKENS_H

# include <stdio.h>

struct tokenbuffer
{
  size_t size;
  char *buffer;
};
typedef struct tokenbuffer token_buffer;

void init_tokenbuffer (token_buffer *tokenbuffer);

size_t
  readtoken (FILE *stream, const char *delim, size_t n_delim,
	     token_buffer *tokenbuffer);
size_t
  readtokens (FILE *stream, size_t projected_n_tokens,
	      const char *delim, size_t n_delim,
	      char ***tokens_out, size_t **token_lengths);

#endif /* not READTOKENS_H */
