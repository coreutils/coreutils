#ifndef _getline_h_
#define _getline_h_ 1

#include <stdio.h>

#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif  /* GCC.  */
#endif  /* Not __P.  */

int
  getline __P ((char **_lineptr, size_t *_n, FILE *_stream));

#endif /* _getline_h_ */
