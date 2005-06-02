#include <config.h>
#include <stddef.h>
#include <errno.h>

/* A trivial substitute for `readlink'.

   DJGPP 2.03 and earlier don't have `readlink' and don't support
   symlinks. */

int
readlink (const char *file, char *buffer, size_t size)
{
  errno = EINVAL;
  return -1;
}
