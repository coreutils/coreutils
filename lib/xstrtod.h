#ifndef XSTRTOD_H
# define XSTRTOD_H 1

#undef PARAMS
#if defined (__STDC__) && __STDC__
# define PARAMS(Args) Args
#else
# define PARAMS(Args) ()
#endif

int
  xstrtod PARAMS ((const char *str, const char **ptr, double *result));

#endif /* not XSTRTOD_H */
