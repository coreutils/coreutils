#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <ctype.h>

#define TOLOWER(Ch) (isupper (Ch) ? tolower (Ch) : (Ch))

/* Compare no more than N characters of S1 and S2,
   ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less
   than, equal to or greater than S2.  */

int
strncasecmp (const char *s1, const char *s2, size_t n)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2 || n == 0)
    return 0;

  do
    {
      c1 = TOLOWER (*p1++);
      c2 = TOLOWER (*p2++);
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
    } while (--n > 0);

  return c1 - c2;
}
