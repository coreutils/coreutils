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

/* Outputs the Unicode character CODE to the output stream STREAM.  */
extern void print_unicode_char PARAMS((FILE *stream, unsigned int code));

#endif
