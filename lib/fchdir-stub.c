#include <config.h>
#include <sys/types.h>
#include <errno.h>

/* A trivial substitute for `fchdir'.

   DJGPP 2.03 and earlier don't have `fchdir'. */

int
fchdir (int fd)
{
  errno = ENOSYS;
  return -1;
}
