#ifndef SAME_H_
# define SAME_H_ 1

# if HAVE_CONFIG_H
#  include <config.h>
# endif

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

int
same_name PARAMS ((const char *source, const char *dest));

#endif /* SAME_H_ */
