#include <stdio.h>
#include <sys/types.h>

size_t
__fpending (FILE *fp)
{
  return PENDING_OUTPUT_N_BYTES;
}
