#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

int
  memcasecmp PARAMS ((const void *vs1, const void *vs2, size_t n));
