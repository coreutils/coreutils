#ifndef XSTRTOD_H
# define XSTRTOD_H 1

# ifndef __P
#  if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#   define __P(Args) Args
#  else
#   define __P(Args) ()
#  endif
# endif

int
  xstrtod __P ((const char *str, const char **ptr, double *result));

#endif /* not XSTRTOD_H */
