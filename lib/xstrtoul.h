#ifndef XSTRTOUL_H_
# define XSTRTOUL_H_ 1

# define STRING_TO_UNSIGNED 1

/* Undefine this symbol so we can include xstrtol.h a second time.
   Otherwise, a program that wanted both xstrtol.h and xstrtoul.h
   would never get the declaration corresponding to the header file 
   included after the first one.  */
# undef XSTRTOL_H_
# include "xstrtol.h"

#endif /* not XSTRTOUL_H_ */
