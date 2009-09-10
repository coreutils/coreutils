/* system-dependent definitions for coreutils
   Copyright (C) 1989, 1991-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
#ifndef ENODATA
# define ENODATA (-1)
#endif

#include <stdbool.h>
#include <stdlib.h>
#include "version.h"

/* Exit statuses for programs like 'env' that exec other programs.  */
enum
{
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

/* include here for SIZE_MAX.  */
#include <inttypes.h>

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
#include "openat.h"

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

/* Return true if DIR is determined to be an empty directory.  */
static inline bool
is_empty_dir (int fd_cwd, char const *dir)
{
  DIR *dirp;
  struct dirent const *dp;
  int saved_errno;
  int fd = openat (fd_cwd, dir,
                   (O_RDONLY | O_DIRECTORY
                    | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK));

  if (fd < 0)
    return false;

  dirp = fdopendir (fd);
  if (dirp == NULL)
    {
      close (fd);
      return false;
    }

  errno = 0;
  dp = readdir_ignoring_dot_and_dotdot (dirp);
  saved_errno = errno;
  closedir (dirp);
  if (dp != NULL)
    return false;
  return saved_errno == 0 ? true : false;
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
#define GETOPT_SELINUX_CONTEXT_OPTION_DECL \
  "context", required_argument, NULL, 'Z'

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

#include "closein.h"
#include "closeout.h"

#define emit_bug_reporting_address unused__emit_bug_reporting_address
#include "version-etc.h"
#undef emit_bug_reporting_address

#include "propername.h"
/* Define away proper_name (leaving proper_name_utf8, which affects far
   fewer programs), since it's not worth the cost of adding ~17KB to
   the x86_64 text size of every single program.  This avoids a 40%
   (almost ~2MB) increase in the on-disk space utilization for the set
   of the 100 binaries. */
#define proper_name(x) (x)

#include "progname.h"

#define case_GETOPT_VERSION_CHAR(Program_name, Authors)			\
  case GETOPT_VERSION_CHAR:						\
    version_etc (stdout, Program_name, PACKAGE_NAME, Version, Authors,	\
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

/* With -Dlint, avoid warnings from gcc about code like mbstate_t m = {0,};
   by wasting space on a static variable of the same type, that is thus
   guaranteed to be initialized to 0, and use that on the RHS.  */
#define DZA_CONCAT0(x,y) x ## y
#define DZA_CONCAT(x,y) DZA_CONCAT0 (x, y)
#ifdef lint
# define DECLARE_ZEROED_AGGREGATE(Type, Var) \
   static Type DZA_CONCAT (s0_, __LINE__); Type Var = DZA_CONCAT (s0_, __LINE__)
#else
# define DECLARE_ZEROED_AGGREGATE(Type, Var) \
  Type Var = { 0, }
#endif

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
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

static inline void
emit_size_note (void)
{
  fputs (_("\n\
SIZE may be (or may be an integer optionally followed by) one of following:\n\
KB 1000, K 1024, MB 1000*1000, M 1024*1024, and so on for G, T, P, E, Z, Y.\n\
"), stdout);
}

static inline void
emit_blocksize_note (char const *program)
{
  printf (_("\n\
Display values are in units of the first available SIZE from --block-size,\n\
and the %s_BLOCK_SIZE, BLOCK_SIZE and BLOCKSIZE environment variables.\n\
Otherwise, units default to 1024 bytes (or 512 if POSIXLY_CORRECT is set).\n\
"), program);
}

#include "hard-locale.h"
static inline void
emit_bug_reporting_address (void)
{
  printf (_("\nReport %s bugs to %s\n"), last_component (program_name),
          PACKAGE_BUGREPORT);
  /* FIXME 2010: use AC_PACKAGE_URL once we require autoconf-2.64 */
  printf (_("%s home page: <http://www.gnu.org/software/%s/>\n"),
          PACKAGE_NAME, PACKAGE);
  fputs (_("General help using GNU software: <http://www.gnu.org/gethelp/>\n"),
         stdout);

  if (hard_locale (LC_MESSAGES))
    {
      /* TRANSLATORS: Replace LANG_CODE in this URL with your language code
         <http://translationproject.org/team/LANG_CODE.html> to form one of
         the URLs at http://translationproject.org/team/.  Otherwise, replace
         the entire URL with your translation team's email address.  */
      printf (_("Report %s translation bugs to "
                "<http://translationproject.org/team/>\n"),
                last_component (program_name));
    }
}

#include "inttostr.h"

static inline char *
timetostr (time_t t, char *buf)
{
  return (TYPE_SIGNED (time_t)
          ? imaxtostr (t, buf)
          : umaxtostr (t, buf));
}

static inline char *
bad_cast (char const *s)
{
  return (char *) s;
}

/* As of Mar 2009, 32KiB is determined to be the minimium
   blksize to best minimize system call overhead.
   This can be tested with this script with the results
   shown for a 1.7GHz pentium-m with 2GB of 400MHz DDR2 RAM:

   for i in $(seq 0 10); do
     size=$((8*1024**3)) #ensure this is big enough
     bs=$((1024*2**$i))
     printf "%7s=" $bs
     dd bs=$bs if=/dev/zero of=/dev/null count=$(($size/$bs)) 2>&1 |
     sed -n 's/.* \([0-9.]* [GM]B\/s\)/\1/p'
   done

      1024=734 MB/s
      2048=1.3 GB/s
      4096=2.4 GB/s
      8192=3.5 GB/s
     16384=3.9 GB/s
     32768=5.2 GB/s
     65536=5.3 GB/s
    131072=5.5 GB/s
    262144=5.7 GB/s
    524288=5.7 GB/s
   1048576=5.8 GB/s

   Note that this is to minimize system call overhead.
   Other values may be appropriate to minimize file system
   or disk overhead.  For example on my current GNU/Linux system
   the readahead setting is 128KiB which was read using:

   file="."
   device=$(df -P --local "$file" | tail -n1 | cut -d' ' -f1)
   echo $(( $(blockdev --getra $device) * 512 ))

   However there isn't a portable way to get the above.
   In the future we could use the above method if available
   and default to io_blksize() if not.
 */
enum { IO_BUFSIZE = 32*1024 };
static inline size_t
io_blksize (struct stat sb)
{
  return MAX (IO_BUFSIZE, ST_BLKSIZE (sb));
}

void usage (int status) ATTRIBUTE_NORETURN;

#ifndef ARRAY_CARDINALITY
# define ARRAY_CARDINALITY(Array) (sizeof (Array) / sizeof *(Array))
#endif
