/* Copy LEN bytes starting at SRCADDR to DESTADDR.  Result undefined
   if the source overlaps with the destination.
   Return DESTADDR. */

#include <assert.h>

/* FIXME: uncomment the following for releases.  */
#define NDEBUG 1

#ifndef ABS
# define ABS(x) ((x) < 0 ? (-(x)) : (x))
#endif

char *
memcpy (destaddr, srcaddr, len)
     char *destaddr;
     const char *srcaddr;
     int len;
{
  char *dest = destaddr;

  assert (len >= 0);
  /* Make sure they don't overlap.  */
  assert (ABS (destaddr - srcaddr) >= len);

  while (len-- > 0)
    *destaddr++ = *srcaddr++;
  return dest;
}
