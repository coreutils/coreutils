#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Try to ensure that each of the standard file numbers (0, 1, 2)
   is in use.  Without this, each application would have to guard
   every call to open, dup, and fopen with tests to ensure they don't
   use one of the special file numbers when opening a user's file.  */
void
stdopen (void)
{
  int fd;
  for (fd = 0; fd <= 2; fd++)
    {
      struct stat sb;
      if (fstat (fd, &sb) < 0)
	{
	  static const int flag[] = { O_WRONLY, O_RDONLY, O_RDONLY };
	  /* FIXME: worry about errno != EBADF?  */
	  char const *file = "/dev/null";
	  int fd2 = open (file, flag[fd]);
	  if (fd2 < 0)
	    break;
	  if (fd2 != fd)
	    {
	      close (fd2);
	      break;
	    }
	}
    }
}
