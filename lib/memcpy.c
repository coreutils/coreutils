/* memcpy.c -- copy memory.
   Copy LENGTH bytes from SOURCE to DEST.  Does not null-terminate.
   The source and destination regions may not overlap.
   In the public domain.
   By Jim Meyering.  */

/* FIXME: remove this before release.  */
#include <assert.h>
#ifndef ABS
# define ABS(x) ((x) < 0 ? (-(x)) : (x))
#endif

void
memcpy (dest, source, length)
     char *dest;
     const char *source;
     unsigned length;
{
  assert (length >= 0);
  /* Make sure they don't overlap.  */
  assert (ABS (dest - source) >= length);

  for (; length; --length)
    *dest++ = *source++;
}
