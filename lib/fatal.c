#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* FIXME: define EXIT_FAILURE */

#include <stdio.h>

#if HAVE_VPRINTF || HAVE_DOPRNT || _LIBC
# if __STDC__
#  include <stdarg.h>
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#  define VA_START(args, lastarg) va_start(args)
# endif
#else
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
#endif

#if STDC_HEADERS || _LIBC
# include <stdlib.h>
# include <string.h>
#else
void exit ();
#endif

#ifdef _LIBC
# define program_name program_invocation_name
#else /* not _LIBC */
/* The calling program should define program_name and set it to the
   name of the executing program.  */
extern char *program_name;
#endif

#include "fatal.h"

/* Like error, but always exit with EXIT_FAILURE.  */

void
#if defined VA_START && __STDC__
fatal (int errnum, const char *message, ...)
#else
fatal (errnum, message, va_alist)
     int errnum;
     char *message;
     va_dcl
#endif
{
#ifdef VA_START
  va_list args;
#endif

  if (error_print_progname)
    (*error_print_progname) ();
  else
    {
      fflush (stdout);
      fprintf (stderr, "%s: ", program_name);
    }

#ifdef VA_START
  VA_START (args, message);
  error (EXIT_FAILURE, errnum, message, args);
  va_end (args);
#else
  error (EXIT_FAILURE, errnum, message, a1, a2, a3, a4, a5, a6, a7, a8);
#endif
}
