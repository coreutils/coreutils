/* Copy LEN bytes starting at SRCADDR to DESTADDR.  Result undefined
   if the source overlaps with the destination.
   Return DESTADDR. */

char *
memcpy (destaddr, srcaddr, len)
     char *destaddr;
     const char *srcaddr;
     int len;
{
  char *dest = destaddr;

  while (len-- > 0)
    *destaddr++ = *srcaddr++;
  return dest;
}
