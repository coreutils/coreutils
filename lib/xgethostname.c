#include <sys/types.h>

int gethostname ();
char *xmalloc ();
char *xrealloc ();

#define INITIAL_HOSTNAME_LENGTH 33

char *
xgethostname ()
{
  char *hostname;
  size_t size;
  int err;

  size = INITIAL_HOSTNAME_LENGTH;
  while (1)
    {
      hostname = xmalloc (size);
      hostname[size - 1] = '\0';
      err = gethostname (hostname, size);
      if (err || hostname[size - 1] == '\0')
	break;
      size *= 2;
      hostname = xrealloc (hostname, size);
    }

  return hostname;
}
