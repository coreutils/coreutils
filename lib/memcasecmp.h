#undef PARAMS
#if defined (__STDC__) && __STDC__
# define PARAMS(args) args
#else
# define PARAMS(args) ()
#endif

int
  memcasecmp PARAMS ((const void *vs1, const void *vs2, size_t n));
