/* strverscmp.h -- compare strings holding indices/version numbers */

#ifndef STRVERSCMP_H_
# define STRVERSCMP_H_

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

int strverscmp PARAMS ((const char*, const char*));

#endif /* not STRVERSCMP_H_ */
