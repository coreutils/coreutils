#include <config.h>

#include <sys/types.h>
#include <stdio.h>

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  ALG_UNSPECIFIED = 0,
  ALG_MD5 = CHAR_MAX + 1,
  ALG_SHA1
};

extern int algorithm;
