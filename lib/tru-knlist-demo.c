/* Derived from the example in the OSF1 knlist manpage.
   OSF1 spe154.testdrive.compaq.com V5.0 1094 alpha
   aka (w/my private hostname compaq-tru64-50a)  */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nlist.h>

#ifndef FD_CLOEXEC
# define FD_CLOEXEC 1
#endif

#ifndef LDAV_SYMBOL
# define LDAV_SYMBOL "_avenrun"
#endif

int
main ()
{
  struct nlist nl[2];
  int retval;
  long offset;

  nl[0].n_name = LDAV_SYMBOL;
  nl[1].n_name = "";

  /*******************************************************/
  /* Call the knlist routine                             */
  retval = knlist (nl);

  /******************************************************/
  /* Display addresses if returned.  Otherwise, display */
  /* the appropriate error message.                     */
  if (retval < 0)
    {
      printf ("No kernel symbol addresses returned.\n");
      exit (1);
    }

  if (nl[0].n_type == 0)
    {
      printf ("Unable to return address of symbol %s\n", nl[0].n_name);
      exit (1);
    }

  offset = nl[0].n_value;
  printf ("The address of symbol %s is %lx\n", nl[0].n_name, offset);

  {
    double load_ave[3];
    int channel = open ("/dev/kmem", 0);
    if (channel < 0)
      {
	printf ("open failed\n");
	exit (1);
      }
#ifdef FD_SETFD
    (void) fcntl (channel, F_SETFD, FD_CLOEXEC);
#endif

    if (lseek (channel, offset, 0) == -1L
	|| read (channel, (char *) load_ave, sizeof (load_ave))
	!= sizeof (load_ave))
      {
	close (channel);
      }
  }

  exit (0);
}
