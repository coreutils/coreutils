#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#if HAVE_STDIO_EXT_H
# include <stdio_ext.h>
#endif

#include <sys/types.h>

size_t __fpending (FILE *);
