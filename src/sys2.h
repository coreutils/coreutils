/* WARNING -- this file is temporary.  It is shared between the
   sh-utils, fileutils, and textutils packages.  Once I find a little
   more time, I'll merge the remaining things in system.h and everything
   in this file will go back there. */

#ifndef RETSIGTYPE
# define RETSIGTYPE void
#endif

#ifndef __GNUC__
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
#   ifdef _WIN32
#    include <malloc.h>
#    include <io.h>
#   else
#    ifndef alloca
char *alloca ();
#    endif
#   endif
#  endif
# endif
#endif

#include <ctype.h>

/* Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."

   Bruno Haible adds:

   "... Furthermore, isupper(c) etc. have an undefined result if c is
   outside the range -1 <= c <= 255. One is tempted to write isupper(c)
   with c being of type `char', but this is wrong if c is an 8-bit
   character >= 128 which gets sign-extended to a negative value.
   The macro ISUPPER protects against this as well."  */

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif

#ifdef isblank
# define ISBLANK(c) (IN_CTYPE_DOMAIN (c) && isblank (c))
#else
# define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif
#ifdef isgraph
# define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isgraph (c))
#else
# define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isprint (c) && !isspace (c))
#endif

#define ISPRINT(c) (IN_CTYPE_DOMAIN (c) && isprint (c))
#define ISALNUM(c) (IN_CTYPE_DOMAIN (c) && isalnum (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISCNTRL(c) (IN_CTYPE_DOMAIN (c) && iscntrl (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))
#define ISPUNCT(c) (IN_CTYPE_DOMAIN (c) && ispunct (c))
#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISUPPER(c) (IN_CTYPE_DOMAIN (c) && isupper (c))
#define ISXDIGIT(c) (IN_CTYPE_DOMAIN (c) && isxdigit (c))
#define ISDIGIT_LOCALE(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))

/* ISDIGIT differs from ISDIGIT_LOCALE, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   Posix 1003.2-1992 section 2.5.2.1 page 50 lines 1556-1558 says that
   only '0' through '9' are digits.  Prefer ISDIGIT to ISDIGIT_LOCALE unless
   it's important to use the locale's definition of `digit' even when the
   host does not conform to Posix.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#ifndef PARAMS
# if PROTOTYPES
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

/* Take care of NLS matters.  */

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(Category, Locale) /* empty */
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) Text
#endif
#define N_(Text) Text

#define STREQ(a,b) (strcmp((a), (b)) == 0)

#ifndef HAVE_DECL_FREE
void free ();
#endif

#ifndef HAVE_DECL_MALLOC
char *malloc ();
#endif

#ifndef HAVE_DECL_MEMCHR
char *memchr ();
#endif

#ifndef HAVE_DECL_REALLOC
char *realloc ();
#endif

#ifndef HAVE_DECL_STPCPY
# ifndef stpcpy
char *stpcpy ();
# endif
#endif

#ifndef HAVE_DECL_STRSTR
char *strstr ();
#endif

#ifndef HAVE_DECL_GETENV
char *getenv ();
#endif

#ifndef HAVE_DECL_LSEEK
off_t lseek ();
#endif

#include "xalloc.h"

#ifndef HAVE_MEMPCPY
/* Be CAREFUL that there are no side effects in N.  */
# define mempcpy(D, S, N) ((void *) ((char *) memcpy (D, S, N) + (N)))
#endif

/* These are wrappers for functions/macros from GNU libc.
   The standard I/O functions are thread-safe.  These *_unlocked ones
   are more efficient but not thread-safe.  That they're not thread-safe
   is fine since all these applications are single threaded.  */

#ifdef HAVE_CLEARERR_UNLOCKED
# undef clearerr
# define clearerr(S) clearerr_unlocked (S)
#endif

#ifdef HAVE_FEOF_UNLOCKED
# undef feof
# define feof(S) feof_unlocked (S)
#endif

#ifdef HAVE_FERROR_UNLOCKED
# undef ferror
# define ferror(S) ferror_unlocked (S)
#endif

#ifdef HAVE_FFLUSH_UNLOCKED
# undef fflush
# define fflush(S) fflush_unlocked (S)
#endif

#ifdef HAVE_FPUTC_UNLOCKED
# undef fputc
# define fputc(C, S) fputc_unlocked (C, S)
#endif

#ifdef HAVE_FREAD_UNLOCKED
# undef fread
# define fread(P, Z, N, S) fread_unlocked (P, Z, N, S)
#endif

#ifdef HAVE_FWRITE_UNLOCKED
# undef fwrite
# define fwrite(P, Z, N, S) fwrite_unlocked (P, Z, N, S)
#endif

#ifdef HAVE_GETC_UNLOCKED
# undef getc
# define getc(S) getc_unlocked (S)
#endif

#ifdef HAVE_GETCHAR_UNLOCKED
# undef getchar
# define getchar(S) getchar_unlocked (S)
#endif

#ifdef HAVE_PUTC_UNLOCKED
# undef putc
# define putc(C, S) putc_unlocked (C, S)
#endif

#ifdef HAVE_PUTCHAR_UNLOCKED
# undef putchar
# define putchar(C) putchar_unlocked (C)
#endif
