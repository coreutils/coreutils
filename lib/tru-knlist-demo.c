/* Derived from the example in the OSF1 knlist manpage.
   OSF1 spe154.testdrive.compaq.com V5.0 1094 alpha
   aka (w/my private hostname compaq-tru64-50a)  */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nlist.h>
main ()
{
  struct nlist   nl[3];
  int            retval, i;
  nl[0].n_name = (char *)malloc(10);
  nl[1].n_name = (char *)malloc(10);
  nl[2].n_name = "";

  /*******************************************************/
  /* Store names of kernel symbols in the nl array       */
  strcpy (nl[0].n_name, "ncpus");
  strcpy (nl[1].n_name, "lockmode");

  /*******************************************************/
  /* Call the knlist routine                             */
  retval = knlist(nl);

  /******************************************************/
  /* Display addresses if returned.  Otherwise, display */
  /* the appropriate error message.                     */
  if (retval < 0)
     printf ("No kernel symbol addresses returned.\n");
  else
     if (retval >= 0 )
	for (i=0; i<2; i++)
	    if (nl[i].n_type == 0)
		printf ("Unable to return address of symbol %s\n",
			nl[i].n_name);
	    else
		printf ("The address of symbol %s is %lx\n",
			nl[i].n_name, nl[i].n_value);
  free (nl[0].n_name);
  free (nl[1].n_name);
}
