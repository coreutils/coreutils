#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

ssize_t
safe_read PARAMS ((int desc, char *ptr, size_t len));
