#ifndef GTOD_H
# define GTOD_H 1

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

void GTOD_init PARAMS ((void));

/* This is a no-op on compliant systems.  */
# if GETTIMEOFDAY_CLOBBERS_LOCALTIME_BUFFER
#  define GETTIMEOFDAY_INIT() GTOD_init ()
# else
#  define GETTIMEOFDAY_INIT() /* empty */
# endif

#endif
