#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
#define ISASCII(c) 1
#else
#define ISASCII(c) isascii(c)
#endif
#define ISUPPER(c) (ISASCII (c) && isupper (c))

#if _LIBC || STDC_HEADERS
# define TOLOWER(c) tolower (c)
#else
# define TOLOWER(c) (ISUPPER (c) ? tolower (c) : (c))
#endif

#include "memcasecmp.h"

/* Like memcmp, but ignore differences in case.  */

int
memcasecmp (vs1, vs2, n)
     const void *vs1;
     const void *vs2;
     size_t n;
{
  unsigned int i;
  unsigned char *s1 = (unsigned char *) vs1;
  unsigned char *s2 = (unsigned char *) vs2;
  for (i = 0; i < n; i++)
    {
      unsigned char u1 = *s1++;
      unsigned char u2 = *s2++;
      if (TOLOWER (u1) != TOLOWER (u2))
        return TOLOWER (u1) - TOLOWER (u2);
    }
  return 0;
}
