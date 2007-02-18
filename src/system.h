/* system-dependent definitions for coreutils
   Copyright (C) 1989, 1991-2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <alloca.h>

/* Include sys/types.h before this file.  */

#if 2 <= __GLIBC__ && 2 <= __GLIBC_MINOR__
# if ! defined _SYS_TYPES_H
you must include <sys/types.h> before including this file
# endif
#endif

#include <sys/stat.h>

#if !defined HAVE_MKFIFO
# define mkfifo(name, mode) mknod (name, (mode) | S_IFIFO, 0)
#endif

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#include <unistd.h>

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif


/* limits.h must come before pathmax.h because limits.h on some systems
   undefs PATH_MAX, whereas pathmax.h sets PATH_MAX.  */
#include <limits.h>

#include "pathmax.h"

#include "configmake.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#if MAJOR_IN_MKDEV
# include <sys/mkdev.h>
# define HAVE_MAJOR
#endif
#if MAJOR_IN_SYSMACROS
# include <sys/sysmacros.h>
# define HAVE_MAJOR
#endif
#ifdef major			/* Might be defined in sys/types.h.  */
# define HAVE_MAJOR
#endif

#ifndef HAVE_MAJOR
# define major(dev)  (((dev) >> 8) & 0xff)
# define minor(dev)  ((dev) & 0xff)
# define makedev(maj, min)  (((maj) << 8) | (min))
#endif
#undef HAVE_MAJOR

#if ! defined makedev && defined mkdev
# define makedev(maj, min)  mkdev (maj, min)
#endif

/* Don't use bcopy!  Use memmove if source and destination may overlap,
   memcpy otherwise.  */

#include <string.h>

#include <errno.h>

/* Some systems don't define the following symbols.  */
#ifndef EDQUOT
# define EDQUOT (-1)
#endif
#ifndef EISDIR
# define EISDIR (-1)
#endif
#ifndef ENOSYS
# define ENOSYS (-1)
#endif

#include <stdbool.h>
#include <stdlib.h>

/* Exit statuses for programs like 'env' that exec other programs.
   EXIT_FAILURE might not be 1, so use EXIT_FAIL in such programs.  */
enum
{
  EXIT_FAIL = 1,
  EXIT_CANNOT_INVOKE = 126,
  EXIT_ENOENT = 127
};

#include "exitfail.h"

/* Set exit_failure to STATUS if that's not the default already.  */
static inline void
initialize_exit_failure (int status)
{
  if (status != EXIT_FAILURE)
    exit_failure = status;
}

#include <fcntl.h>

#ifndef F_OK
# define F_OK 0
# define X_OK 1
# define W_OK 2
# define R_OK 4
#endif

#include <dirent.h>
#ifndef _D_EXACT_NAMLEN
# define _D_EXACT_NAMLEN(dp) strlen ((dp)->d_name)
#endif

enum
{
  NOT_AN_INODE_NUMBER = 0
};

#ifdef D_INO_IN_DIRENT
# define D_INO(dp) (dp)->d_ino
#else
/* Some systems don't have inodes, so fake them to avoid lots of ifdefs.  */
# define D_INO(dp) NOT_AN_INODE_NUMBER
#endif

/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#if !defined DEV_BSIZE && defined BSIZE
# define DEV_BSIZE BSIZE
#endif
#if !defined DEV_BSIZE && defined BBSIZE /* SGI */
# define DEV_BSIZE BBSIZE
#endif
#ifndef DEV_BSIZE
# define DEV_BSIZE 4096
#endif

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
   ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined _POSIX_SOURCE || !defined BSIZE /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) \
  ((statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0))
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? st_blocks ((statbuf).st_size) : 0)
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_STRUCT_STAT_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes.
   Also, when running `rsh hpux11-system cat any-file', cat would
   determine that the output stream had an st_blksize of 2147421096.
   Conversely st_blksize can be 2 GiB (or maybe even larger) with XFS
   on 64-bit hosts.  Somewhat arbitrarily, limit the `optimal' block
   size to SIZE_MAX / 8 + 1.  (Dividing SIZE_MAX by only 4 wouldn't
   suffice, since "cat" sometimes multiplies the result by 4.)  If
   anyone knows of a system for which this limit is too small, please
   report it as a bug in this code.  */
# define ST_BLKSIZE(statbuf) ((0 < (statbuf).st_blksize \
			       && (statbuf).st_blksize <= SIZE_MAX / 8 + 1) \
			      ? (statbuf).st_blksize : DEV_BSIZE)
# if defined hpux || defined __hpux__ || defined __hpux
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD file systems with NFS.  */
#  define ST_NBLOCKSIZE 1024
# else /* !hpux */
#  if defined _AIX && defined _I386
/* AIX PS/2 counts st_blocks in 4K units.  */
#   define ST_NBLOCKSIZE (4 * 1024)
#  else /* not AIX PS/2 */
#   if defined _CRAY
#    define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
#   endif /* _CRAY */
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */

#ifndef ST_NBLOCKS
# define ST_NBLOCKS(statbuf) ((statbuf).st_blocks)
#endif

#ifndef ST_NBLOCKSIZE
# ifdef S_BLKSIZE
#  define ST_NBLOCKSIZE S_BLKSIZE
# else
#  define ST_NBLOCKSIZE 512
# endif
#endif

/* Redirection and wildcarding when done by the utility itself.
   Generally a noop, but used in particular for native VMS. */
#ifndef initialize_main
# define initialize_main(ac, av)
#endif

#include "stat-macros.h"

#include "timespec.h"

#include <inttypes.h>

#include <ctype.h>

#if ! (defined isblank || HAVE_DECL_ISBLANK)
# define isblank(c) ((c) == ' ' || (c) == '\t')
#endif

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char
     or EOF.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   isdigit unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned int) (c) - '0' <= 9)

/* Convert a possibly-signed character to an unsigned character.  This is
   a bit safer than casting to unsigned char, since it catches some type
   errors that the cast doesn't.  */
static inline unsigned char to_uchar (char ch) { return ch; }

#include <locale.h>

/* Take care of NLS matters.  */

#include "gettext.h"
#if ! ENABLE_NLS
# undef textdomain
# define textdomain(Domainname) /* empty */
# undef bindtextdomain
# define bindtextdomain(Domainname, Dirname) /* empty */
#endif

#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

/* Return a value that pluralizes the same way that N does, in all
   languages we know of.  */
static inline unsigned long int
select_plural (uintmax_t n)
{
  /* Reduce by a power of ten, but keep it away from zero.  The
     gettext manual says 1000000 should be safe.  */
  enum { PLURAL_REDUCER = 1000000 };
  return (n <= ULONG_MAX ? n : n % PLURAL_REDUCER + PLURAL_REDUCER);
}

#define STREQ(a, b) (strcmp (a, b) == 0)

#if !HAVE_DECL_FREE
void free ();
#endif

#if !HAVE_DECL_MALLOC
char *malloc ();
#endif

#if !HAVE_DECL_MEMCHR
char *memchr ();
#endif

#if !HAVE_DECL_REALLOC
char *realloc ();
#endif

#if !HAVE_DECL_GETENV
char *getenv ();
#endif

#if !HAVE_DECL_LSEEK
off_t lseek ();
#endif

#if !HAVE_DECL_GETLOGIN
char *getlogin ();
#endif

#if !HAVE_DECL_TTYNAME
char *ttyname ();
#endif

#if !HAVE_DECL_GETEUID
uid_t geteuid ();
#endif

#if !HAVE_DECL_GETPWUID
struct passwd *getpwuid ();
#endif

#if !HAVE_DECL_GETGRGID
struct group *getgrgid ();
#endif

#if !HAVE_DECL_GETUID
uid_t getuid ();
#endif

#include "xalloc.h"
#include "verify.h"

/* This is simply a shorthand for the common case in which
   the third argument to x2nrealloc would be `sizeof *(P)'.
   Ensure that sizeof *(P) is *not* 1.  In that case, it'd be
   better to use X2REALLOC, although not strictly necessary.  */
#define X2NREALLOC(P, PN) ((void) verify_true (sizeof *(P) != 1), \
			   x2nrealloc (P, PN, sizeof *(P)))

/* Using x2realloc (when appropriate) usually makes your code more
   readable than using x2nrealloc, but it also makes it so your
   code will malfunction if sizeof *(P) ever becomes 2 or greater.
   So use this macro instead of using x2realloc directly.  */
#define X2REALLOC(P, PN) ((void) verify_true (sizeof *(P) == 1), \
                          x2realloc (P, PN))

#include "unlocked-io.h"
#include "same-inode.h"

#include "dirname.h"

static inline bool
dot_or_dotdot (char const *file_name)
{
  if (file_name[0] == '.')
    {
      char sep = file_name[(file_name[1] == '.') + 1];
      return (! sep || ISSLASH (sep));
    }
  else
    return false;
}

/* A wrapper for readdir so that callers don't see entries for `.' or `..'.  */
static inline struct dirent const *
readdir_ignoring_dot_and_dotdot (DIR *dirp)
{
  while (1)
    {
      struct dirent const *dp = readdir (dirp);
      if (dp == NULL || ! dot_or_dotdot (dp->d_name))
	return dp;
    }
}

/* Factor out some of the common --help and --version processing code.  */

/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */
enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, NULL, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, NULL, GETOPT_VERSION_CHAR

#define case_GETOPT_HELP_CHAR			\
  case GETOPT_HELP_CHAR:			\
    usage (EXIT_SUCCESS);			\
    break;

/* Program_name must be a literal string.
   Usually it is just PROGRAM_NAME.  */
#define USAGE_BUILTIN_WARNING \
  _("\n" \
"NOTE: your shell may have its own version of %s, which usually supersedes\n" \
"the version described here.  Please refer to your shell's documentation\n" \
"for details about the options it supports.\n")

#define HELP_OPTION_DESCRIPTION \
  _("      --help     display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version  output version information and exit\n")

#include "closeout.h"
#include "version-etc.h"

#define case_GETOPT_VERSION_CHAR(Program_name, Authors)			\
  case GETOPT_VERSION_CHAR:						\
    version_etc (stdout, Program_name, GNU_PACKAGE, VERSION, Authors,	\
                 (char *) NULL);					\
    exit (EXIT_SUCCESS);						\
    break;

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#include "intprops.h"

#ifndef SSIZE_MAX
# define SSIZE_MAX TYPE_MAXIMUM (ssize_t)
#endif

#ifndef OFF_T_MIN
# define OFF_T_MIN TYPE_MINIMUM (off_t)
#endif

#ifndef OFF_T_MAX
# define OFF_T_MAX TYPE_MAXIMUM (off_t)
#endif

#ifndef UID_T_MAX
# define UID_T_MAX TYPE_MAXIMUM (uid_t)
#endif

#ifndef GID_T_MAX
# define GID_T_MAX TYPE_MAXIMUM (gid_t)
#endif

#ifndef PID_T_MAX
# define PID_T_MAX TYPE_MAXIMUM (pid_t)
#endif

/* Use this to suppress gcc's `...may be used before initialized' warnings. */
#ifdef lint
# define IF_LINT(Code) Code
#else
# define IF_LINT(Code) /* empty */
#endif

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
#  define __attribute__(x) /* empty */
# endif
#endif

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

#if defined strdupa
# define ASSIGN_STRDUPA(DEST, S)		\
  do { DEST = strdupa (S); } while (0)
#else
# define ASSIGN_STRDUPA(DEST, S)		\
  do						\
    {						\
      const char *s_ = (S);			\
      size_t len_ = strlen (s_) + 1;		\
      char *tmp_dest_ = alloca (len_);		\
      DEST = memcpy (tmp_dest_, s_, len_);	\
    }						\
  while (0)
#endif

#ifndef EOVERFLOW
# define EOVERFLOW EINVAL
#endif

#if ! HAVE_FSEEKO
# if ! defined fseeko
#  define fseeko(s, o, w) ((o) == (long int) (o)	\
			   ? fseek (s, o, w)		\
			   : (errno = EOVERFLOW, -1))
# endif
# if ! defined ftello
static inline off_t ftello (FILE *stream)
{
  verify (sizeof (long int) <= sizeof (off_t));
  return ftell (stream);
}
# endif
#endif

#if ! HAVE_SYNC
# define sync() /* empty */
#endif

/* Compute the greatest common divisor of U and V using Euclid's
   algorithm.  U and V must be nonzero.  */

static inline size_t
gcd (size_t u, size_t v)
{
  do
    {
      size_t t = u % v;
      u = v;
      v = t;
    }
  while (v);

  return u;
}

/* Compute the least common multiple of U and V.  U and V must be
   nonzero.  There is no overflow checking, so callers should not
   specify outlandish sizes.  */

static inline size_t
lcm (size_t u, size_t v)
{
  return u * (v / gcd (u, v));
}

/* Return PTR, aligned upward to the next multiple of ALIGNMENT.
   ALIGNMENT must be nonzero.  The caller must arrange for ((char *)
   PTR) through ((char *) PTR + ALIGNMENT - 1) to be addressable
   locations.  */

static inline void *
ptr_align (void const *ptr, size_t alignment)
{
  char const *p0 = ptr;
  char const *p1 = p0 + alignment - 1;
  return (void *) (p1 - (size_t) p1 % alignment);
}

/* If 10*Accum + Digit_val is larger than the maximum value for Type,
   then don't update Accum and return false to indicate it would
   overflow.  Otherwise, set Accum to that new value and return true.
   Verify at compile-time that Type is Accum's type, and that Type is
   unsigned.  Accum must be an object, so that we can take its
   address.  Accum and Digit_val may be evaluated multiple times.

   The "Added check" below is not strictly required, but it causes GCC
   to return a nonzero exit status instead of merely a warning
   diagnostic, and that is more useful.  */

#define DECIMAL_DIGIT_ACCUMULATE(Accum, Digit_val, Type)		\
  (									\
   (void) (&(Accum) == (Type *) NULL),  /* The type matches.  */	\
   (void) verify_true (! TYPE_SIGNED (Type)), /* The type is unsigned.  */ \
   (void) verify_true (sizeof (Accum) == sizeof (Type)), /* Added check.  */ \
   (((Type) -1 / 10 < (Accum)						\
     || (Type) ((Accum) * 10 + (Digit_val)) < (Accum))			\
    ? false : (((Accum) = (Accum) * 10 + (Digit_val)), true))		\
  )
