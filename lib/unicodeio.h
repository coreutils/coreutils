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

#ifndef UNICODEIO_H
# define UNICODEIO_H

# include <stdio.h>

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

/* Outputs the Unicode character CODE to the output stream STREAM.
   Upon failure, exit if exit_on_error is true, otherwise output a fallback
   notation.  */
extern void print_unicode_char PARAMS ((FILE *stream, unsigned int code,
					int exit_on_error));

/* Simple success callback that outputs the converted string.
   The STREAM is passed as callback_arg.  */
extern long fwrite_success_callback PARAMS ((const char *buf, size_t buflen,
					     void *callback_arg));

#endif
