#if ! defined PATH_CONCAT_H_
# define PATH_CONCAT_H_

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

char *
path_concat PARAMS ((const char *dir, const char *base, char **base_in_result));

#endif
