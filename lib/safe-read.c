/* Read LEN bytes at PTR from descriptor DESC, retrying if necessary.
   Return the actual number of bytes read, zero for EOF, or negative
   for an error.  */

int
safe_read (desc, ptr, len)
     int desc;
     char *ptr;
     int len;
{
  int n_remaining;

  n_remaining = len;
  while (n_remaining > 0)
    {
      int n_chars = read (desc, ptr, n_remaining);
      if (n_chars < 0)
	{
#ifdef EINTR
	  if (errno == EINTR)
	    continue;
#endif
	  return n_chars;
	}
      if (n_chars == 0)
	break;
      ptr += n_chars;
      n_remaining -= n_chars;
    }
  return len - n_remaining;
}
