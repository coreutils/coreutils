#include <config.h>
#include <sys/stat.h>

/* A trivial substitute for `lstat'.

   DJGPP 2.03 and earlier don't have `lstat' and don't support
   symlinks. */

int
lstat (const char *fname, struct stat *st_buf)
{
  return stat (fname, st_buf);
}
