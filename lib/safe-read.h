#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

#define SAFE_READ_ERROR ((size_t) -1)

size_t
safe_read PARAMS ((int desc, void *ptr, size_t len));
