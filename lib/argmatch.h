#ifndef ARGMATCH_H
#define ARGMATCH_H 1

#ifndef __P
# if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#  define __P(args) args
# else
#  define __P(args) ()
# endif  /* GCC.  */
#endif  /* Not __P.  */

int
  argmatch __P ((const char *arg, const char *const *optlist));

void
  invalid_arg __P ((const char *kind, const char *value, int problem));

#endif /* ARGMATCH_H */
