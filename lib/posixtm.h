#ifndef POSIXTM_H_
# define POSIXTM_H_

/* POSIX Date Syntax flags.  */
# define PDS_LEADING_YEAR 1
# define PDS_TRAILING_YEAR 2
# define PDS_CENTURY 4
# define PDS_SECONDS 8

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

bool
posixtime PARAMS ((time_t *p, const char *s, unsigned int syntax_bits));

#endif
