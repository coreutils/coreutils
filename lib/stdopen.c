#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/* Try to ensure that each of the standard file numbers (0, 1, 2)
   is in use.  Without this, each application would have to guard
   every call to open, dup, and fopen with tests to ensure they don't
   use one of the special file numbers when opening a user's file.  */
void
stdopen (void)
{
  int fd = dup (STDIN_FILENO);

  if (0 <= fd)
    close (fd);
  else if (errno == EBADF)
    fd = open ("/dev/null", O_WRONLY);

  if (STDIN_FILENO <= fd && fd <= STDERR_FILENO)
    {
      fd = open ("/dev/null", O_RDONLY);
      if (fd == STDOUT_FILENO)
	fd = dup (fd);
      if (STDERR_FILENO < fd)
	close (fd);
    }
}
