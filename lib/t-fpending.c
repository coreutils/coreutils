/* Ensure that __fpending works.  */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include "__fpending.h"

int
main ()
{
  if (__fpending (stdout) != 0)
    abort ();

  fputs ("foo", stdout);
  if (__fpending (stdout) != 3)
    abort ();

  fflush (stdout);
  if (__fpending (stdout) != 0)
    abort ();

  exit (0);
}
