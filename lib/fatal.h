#include "error.h"

/* FIXME: this is all from ansidecl.  better to simply swipe
   that file from egcs/include and include it from here.  */

/* Using MACRO(x,y) in cpp #if conditionals does not work with some
   older preprocessors.  Thus we can't define something like this:

#define HAVE_GCC_VERSION(MAJOR, MINOR) \
  (__GNUC__ > (MAJOR) || (__GNUC__ == (MAJOR) && __GNUC_MINOR__ >= (MINOR)))

and then test "#if HAVE_GCC_VERSION(2,7)".

So instead we use the macro below and test it against specific values.  */

/* This macro simplifies testing whether we are using gcc, and if it
   is of a particular minimum version. (Both major & minor numbers are
   significant.)  This macro will evaluate to 0 if we are not using
   gcc at all.  */
#ifndef GCC_VERSION
# define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#endif /* GCC_VERSION */

/* Define macros for some gcc attributes.  This permits us to use the
   macros freely, and know that they will come into play for the
   version of gcc in which they are supported.  */

#if (GCC_VERSION < 2007)
# define __attribute__(x)
#endif

/* Attribute __malloc__ on functions was valid as of gcc 2.96. */
#ifndef ATTRIBUTE_MALLOC
# if (GCC_VERSION >= 2096)
#  define ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
# else
#  define ATTRIBUTE_MALLOC
# endif /* GNUC >= 2.96 */
#endif /* ATTRIBUTE_MALLOC */

/* Attributes on labels were valid as of gcc 2.93. */
#ifndef ATTRIBUTE_UNUSED_LABEL
# if (GCC_VERSION >= 2093)
#  define ATTRIBUTE_UNUSED_LABEL ATTRIBUTE_UNUSED
# else
#  define ATTRIBUTE_UNUSED_LABEL
# endif /* GNUC >= 2.93 */
#endif /* ATTRIBUTE_UNUSED_LABEL */

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif /* ATTRIBUTE_UNUSED */

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif /* ATTRIBUTE_NORETURN */

#ifndef ATTRIBUTE_PRINTF
# define ATTRIBUTE_PRINTF(m, n) __attribute__ ((__format__ (__printf__, m, n)))
# define ATTRIBUTE_PRINTF_1 ATTRIBUTE_PRINTF(1, 2)
# define ATTRIBUTE_PRINTF_2 ATTRIBUTE_PRINTF(2, 3)
# define ATTRIBUTE_PRINTF_3 ATTRIBUTE_PRINTF(3, 4)
# define ATTRIBUTE_PRINTF_4 ATTRIBUTE_PRINTF(4, 5)
# define ATTRIBUTE_PRINTF_5 ATTRIBUTE_PRINTF(5, 6)
#endif /* ATTRIBUTE_PRINTF */

extern void fatal (int errnum, const char *format, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF_2;
