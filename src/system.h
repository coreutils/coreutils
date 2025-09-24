/* system-dependent definitions for coreutils
   Copyright (C) 1989-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Include this file _after_ system headers if possible.  */

#include <attribute.h>

#include <alloca.h>

#include <sys/stat.h>

/* Commonly used file permission combination.  */
#define MODE_RW_UGO (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#include <unistd.h>

#include <limits.h>

#include "pathmax.h"
#ifndef PATH_MAX
# define PATH_MAX 8192
#endif

#include "configmake.h"

#include <sys/time.h>
#include <time.h>

/* Since major is a function on SVR4, we can't use 'ifndef major'.  */
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

#include <stdckdint.h>
#include <stdcountof.h>
#include <stddef.h>
#include <string.h>
#include <uchar.h>
#include <errno.h>

/* Some systems don't define this; POSIX mentions it but says it is
   obsolete.  gnulib defines it, but only on native Windows systems,
   and there only because MSVC 10 does.  */
#ifndef ENODATA
# define ENODATA (-1)
#endif

#include <stdlib.h>
#include "version.h"

/* Exit statuses for programs like 'env' that exec other programs.  */
enum
{
  EXIT_TIMEDOUT = 124, /* Time expired before child completed.  */
  EXIT_CANCELED = 125, /* Internal error prior to exec attempt.  */
  EXIT_CANNOT_INVOKE = 126, /* Program located, but not usable.  */
  EXIT_ENOENT = 127 /* Could not find program to exec.  */
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
#ifdef O_PATH
enum { O_PATHSEARCH = O_PATH };
#else
enum { O_PATHSEARCH = O_SEARCH };
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

/* Redirection and wildcarding when done by the utility itself.
   Generally a noop, but used in particular for OS/2.  */
#ifndef initialize_main
# ifndef __OS2__
#  define initialize_main(ac, av)
# else
#  define initialize_main(ac, av) \
     do { _wildcard (ac, av); _response (ac, av); } while (0)
# endif
#endif

#include "stat-macros.h"

#include "timespec.h"

/* Convert a possibly-signed character to an unsigned character.  This is
   a bit safer than casting to unsigned char, since it catches some type
   errors that the cast doesn't.  */
static inline unsigned char to_uchar (char ch) { return ch; }

/* Return non zero if a non breaking space.  */
ATTRIBUTE_PURE
static inline int
c32isnbspace (char32_t wc)
{
  return wc == 0x00A0 || wc == 0x2007 || wc == 0x202F || wc == 0x2060;
}

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

#define STREQ_LEN(a, b, n) (strncmp (a, b, n) == 0) /* n==-1 means unbounded */
#define STRPREFIX(a, b) (strncmp (a, b, strlen (b)) == 0)

/* Just like strncmp, but the second argument must be a literal string
   and you don't specify the length;  that comes from the literal.  */
#define STRNCMP_LIT(s, lit) strncmp (s, "" lit "", sizeof (lit) - 1)

#if !HAVE_DECL_GETLOGIN
char *getlogin (void);
#endif

#if !HAVE_DECL_TTYNAME
char *ttyname (int);
#endif

#if !HAVE_DECL_GETEUID
uid_t geteuid (void);
#endif

#if !HAVE_DECL_GETPWUID
struct passwd *getpwuid (uid_t);
#endif

#if !HAVE_DECL_GETGRGID
struct group *getgrgid (gid_t);
#endif

/* Interix has replacements for getgr{gid,nam,ent}, that don't
   query the domain controller for group members when not required.
   This speeds up the calls tremendously (<1 ms vs. >3 s). */
/* To protect any system that could provide _nomembers functions
   other than interix, check for HAVE_SETGROUPS, as interix is
   one of the very few (the only?) platform that lacks it */
#if ! HAVE_SETGROUPS
# if HAVE_GETGRGID_NOMEMBERS
#  define getgrgid(gid) getgrgid_nomembers(gid)
# endif
# if HAVE_GETGRNAM_NOMEMBERS
#  define getgrnam(nam) getgrnam_nomembers(nam)
# endif
# if HAVE_GETGRENT_NOMEMBERS
#  define getgrent() getgrent_nomembers()
# endif
#endif

#if !HAVE_DECL_GETUID
uid_t getuid (void);
#endif

#include "idx.h"
#include "xalloc.h"
#include "verify.h"
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

/* A wrapper for readdir so that callers don't see entries for '.' or '..'.  */
static inline struct dirent const *
readdir_ignoring_dot_and_dotdot (DIR *dirp)
{
  while (true)
    {
      struct dirent const *dp = readdir (dirp);
      if (dp == nullptr || ! dot_or_dotdot (dp->d_name))
        return dp;
    }
}

/* Return -1 if DIR is an empty directory,
   0 if DIR is a nonempty directory,
   and a positive error number if there was trouble determining
   whether DIR is an empty or nonempty directory.  */
enum {
    DS_UNKNOWN = -2,
    DS_EMPTY = -1,
    DS_NONEMPTY = 0,
};
static inline int
directory_status (int fd_cwd, char const *dir)
{
  DIR *dirp;
  bool no_direntries;
  int saved_errno;
  int fd = openat (fd_cwd, dir,
                   (O_RDONLY | O_DIRECTORY
                    | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK));

  if (fd < 0)
    return errno;

  dirp = fdopendir (fd);
  if (dirp == nullptr)
    {
      saved_errno = errno;
      close (fd);
      return saved_errno;
    }

  errno = 0;
  no_direntries = !readdir_ignoring_dot_and_dotdot (dirp);
  saved_errno = errno;
  closedir (dirp);
  return no_direntries && saved_errno == 0 ? DS_EMPTY : saved_errno;
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
  "help", no_argument, nullptr, GETOPT_HELP_CHAR
#define GETOPT_VERSION_OPTION_DECL \
  "version", no_argument, nullptr, GETOPT_VERSION_CHAR
#define GETOPT_SELINUX_CONTEXT_OPTION_DECL \
  "context", optional_argument, nullptr, 'Z'

#define case_GETOPT_HELP_CHAR			\
  case GETOPT_HELP_CHAR:			\
    usage (EXIT_SUCCESS);			\
    break;

/* Program_name must be a literal string.
   Usually it is just PROGRAM_NAME.  */
#define USAGE_BUILTIN_WARNING \
  _("\n" \
"Your shell may have its own version of %s, which usually supersedes\n" \
"the version described here.  Please refer to your shell's documentation\n" \
"for details about the options it supports.\n")

#define HELP_OPTION_DESCRIPTION \
  _("      --help        display this help and exit\n")
#define VERSION_OPTION_DESCRIPTION \
  _("      --version     output version information and exit\n")

#include "closein.h"
#include "closeout.h"

#include "version-etc.h"

#include "propername.h"
/* Define away proper_name, since it's not worth the cost of adding ~17KB to
   the x86_64 text size of every single program.  This avoids a 40%
   (almost ~2MB) increase in the file system space utilization for the set
   of the 100 binaries. */
#define proper_name(x) proper_name_lite (x, x)

#include "progname.h"

#define case_GETOPT_VERSION_CHAR(Program_name, Authors)			\
  case GETOPT_VERSION_CHAR:						\
    version_etc (stdout, Program_name, PACKAGE_NAME, Version, Authors,	\
                 (char *) nullptr);					\
    exit (EXIT_SUCCESS);						\
    break;

#include "minmax.h"
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

/* Use this to suppress gcc warnings.  */
#ifdef lint
# define IF_LINT(Code) Code
#else
# define IF_LINT(Code) /* empty */
#endif

/* main_exit should be called only from the main function.  It is
   equivalent to 'exit'.  When checking for lint it calls 'exit', to
   pacify gcc -fsanitize=lint which would otherwise have false alarms
   for pointers in the main function's activation record.  Otherwise
   it simply returns from 'main'; this used to be what gcc's static
   checking preferred and may yet be again.  */
#ifdef lint
# define main_exit(status) exit (status)
#else
# define main_exit(status) return status
#endif

#ifdef __GNUC__
# define LIKELY(cond)    __builtin_expect ((cond), 1)
# define UNLIKELY(cond)  __builtin_expect ((cond), 0)
#else
# define LIKELY(cond)    (cond)
# define UNLIKELY(cond)  (cond)
#endif


#if defined strdupa
# define ASSIGN_STRDUPA(DEST, S)		\
  do { DEST = strdupa (S); } while (0)
#else
# define ASSIGN_STRDUPA(DEST, S)		\
  do						\
    {						\
      char const *s_ = (S);			\
      size_t len_ = strlen (s_) + 1;		\
      char *tmp_dest_ = alloca (len_);		\
      DEST = memcpy (tmp_dest_, s_, len_);	\
    }						\
  while (0)
#endif

#if ! HAVE_SYNC
# define sync() /* empty */
#endif

/* Compute the greatest common divisor of U and V using Euclid's
   algorithm.  U and V must be nonzero.  */

ATTRIBUTE_CONST
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

ATTRIBUTE_CONST
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

/* Return whether the buffer consists entirely of NULs.
   Based on memeqzero in CCAN by Rusty Russell under CC0 (Public domain).  */

ATTRIBUTE_PURE
static inline bool
is_nul (void const *buf, size_t length)
{
  const unsigned char *p = buf;
/* Using possibly unaligned access for the first 16 bytes
   saves about 30-40 cycles, though it is strictly undefined behavior
   and so would need __attribute__ ((__no_sanitize_undefined__))
   to avoid -fsanitize=undefined warnings.
   Considering coreutils is mainly concerned with relatively
   large buffers, we'll just use the defined behavior.  */
#if 0 && (_STRING_ARCH_unaligned || _STRING_INLINE_unaligned)
  unsigned long word;
#else
  unsigned char word;
#endif

  if (! length)
    return true;

  /* Check len bytes not aligned on a word.  */
  while (UNLIKELY (length & (sizeof word - 1)))
    {
      if (*p)
        return false;
      p++;
      length--;
      if (! length)
        return true;
   }

  /* Check up to 16 bytes a word at a time.  */
  for (;;)
    {
      memcpy (&word, p, sizeof word);
      if (word)
        return false;
      p += sizeof word;
      length -= sizeof word;
      if (! length)
        return true;
      if (UNLIKELY (length & 15) == 0)
        break;
   }

  /* Now we know first 16 bytes are NUL, memeq with self.  */
  return memeq (buf, p, length);
}

/* Set Accum = 10*Accum + Digit_val and return true, where Accum is an
   integer object and Digit_val an integer expression.  However, if
   the result overflows, set Accum to an unspecified value and return
   false.  Accum and Digit_val may be evaluated multiple times.  */

#define DECIMAL_DIGIT_ACCUMULATE(Accum, Digit_val)			\
  (!ckd_mul (&(Accum), Accum, 10) && !ckd_add (&(Accum), Accum, Digit_val))

static inline void
emit_stdin_note (void)
{
  fputs (_("\n\
With no FILE, or when FILE is -, read standard input.\n\
"), stdout);
}
static inline void
emit_mandatory_arg_note (void)
{
  fputs (_("\n\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
}

static inline void
emit_size_note (void)
{
  fputs (_("\n\
The SIZE argument is an integer and optional unit (example: 10K is 10*1024).\n\
Units are K,M,G,T,P,E,Z,Y,R,Q (powers of 1024) or KB,MB,... (powers of 1000).\n\
Binary prefixes can be used, too: KiB=K, MiB=M, and so on.\n\
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

static inline void
emit_update_parameters_note (void)
{
  fputs (_("\
\n\
UPDATE controls which existing files in the destination are replaced.\n\
'all' is the default operation when an --update option is not specified,\n\
and results in all existing files in the destination being replaced.\n\
'none' is like the --no-clobber option, in that no files in the\n\
destination are replaced, and skipped files do not induce a failure.\n\
'none-fail' also ensures no files are replaced in the destination,\n\
but any skipped files are diagnosed and induce a failure.\n\
'older' is the default operation when --update is specified, and results\n\
in files being replaced if they're older than the corresponding source file.\n\
"), stdout);
}

static inline void
emit_backup_suffix_note (void)
{
  fputs (_("\
\n\
The backup suffix is '~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control method may be selected via the --backup option or through\n\
the VERSION_CONTROL environment variable.  Here are the values:\n\
\n\
"), stdout);
  fputs (_("\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n\
"), stdout);
}

static inline void
emit_symlink_recurse_options (char const *default_opt)
{
      printf (_("\
\n\
The following options modify how a hierarchy is traversed when the -R\n\
option is also specified.  If more than one is specified, only the final\n\
one takes effect. %s is the default.\n\
\n\
  -H                     if a command line argument is a symbolic link\n\
                         to a directory, traverse it\n\
  -L                     traverse every symbolic link to a directory\n\
                         encountered\n\
  -P                     do not traverse any symbolic links\n\
\n\
"), default_opt);
}

static inline void
emit_exec_status (char const *program)
{
      printf (_("\n\
Exit status:\n\
  125  if the %s command itself fails\n\
  126  if COMMAND is found but cannot be invoked\n\
  127  if COMMAND cannot be found\n\
  -    the exit status of COMMAND otherwise\n\
"), program);
}

static inline void
emit_ancillary_info (char const *program)
{
  struct infomap { char const *program; char const *node; } const infomap[] = {
    { "[", "test invocation" },
    { "coreutils", "Multi-call invocation" },
    { "sha224sum", "sha2 utilities" },
    { "sha256sum", "sha2 utilities" },
    { "sha384sum", "sha2 utilities" },
    { "sha512sum", "sha2 utilities" },
    { nullptr, nullptr }
  };

  char const *node = program;
  struct infomap const *map_prog = infomap;

  while (map_prog->program && ! streq (program, map_prog->program))
    map_prog++;

  if (map_prog->node)
    node = map_prog->node;

  emit_bug_reporting_address ();

  /* Don't output this redundant message for English locales.
     Note we still output for 'C' so that it gets included in the man page.  */
  char const *lc_messages = setlocale (LC_MESSAGES, nullptr);
  if (lc_messages && STRNCMP_LIT (lc_messages, "en_"))
    {
      /* TRANSLATORS: Replace LANG_CODE in this URL with your language code
         <https://translationproject.org/team/LANG_CODE.html> to form one of
         the URLs at https://translationproject.org/team/.  Otherwise, replace
         the entire URL with your translation team's email address.  */
      fputs (_("Report any translation bugs to "
               "<https://translationproject.org/team/>\n"), stdout);
    }
  /* .htaccess on the coreutils web site maps programs to the appropriate page,
     however we explicitly handle "[" -> "test" here as the "[" is not
     recognized as part of a URL by default in terminals.  */
  char const *url_program = streq (program, "[") ? "test" : program;
  printf (_("Full documentation <%s%s>\n"),
          PACKAGE_URL, url_program);
  printf (_("or available locally via: info '(coreutils) %s%s'\n"),
          node, node == program ? " invocation" : "");
}

/* Use a macro rather than an inline function, as this references
   the global program_name, which causes dynamic linking issues
   in libstdbuf.so on some systems where unused functions
   are not removed by the linker.  */
#define emit_try_help() \
  do \
    { \
      fprintf (stderr, _("Try '%s --help' for more information.\n"), \
               program_name); \
    } \
  while (0)

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

/* Return a boolean indicating whether SB->st_size is defined.  */
static inline bool
usable_st_size (struct stat const *sb)
{
  return (S_ISREG (sb->st_mode) || S_ISLNK (sb->st_mode)
          || S_TYPEISSHM (sb) || S_TYPEISTMO (sb));
}

_Noreturn void usage (int status);

#include <error.h>

/* Like error(0, 0, ...), but without an implicit newline.
   Also a noop unless the global DEV_DEBUG is set.  */
#define devmsg(...)			\
  do					\
    {					\
      if (dev_debug)			\
        fprintf (stderr, __VA_ARGS__);	\
    }					\
  while (0)

#define emit_cycle_warning(file_name)	\
  do					\
    {					\
      error (0, 0, _("\
WARNING: Circular directory structure.\n\
This almost certainly means that you have a corrupted file system.\n\
NOTIFY YOUR SYSTEM MANAGER.\n\
The following directory is part of the cycle:\n  %s\n"), \
             quotef (file_name));	\
    }					\
  while (0)

/* exit with a _single_ "write error" diagnostic.  */

static inline void
write_error (void)
{
  int saved_errno = errno;
  fflush (stdout);    /* Last attempt to write any buffered data.  */
  fpurge (stdout);    /* Ensure nothing buffered that might induce an error. */
  clearerr (stdout);  /* Avoid extraneous diagnostic from close_stdout.  */
  error (EXIT_FAILURE, saved_errno, _("write error"));
}

/* Like stpncpy, but do ensure that the result is NUL-terminated,
   and do not NUL-pad out to LEN.  I.e., when strnlen (src, len) == len,
   this function writes a NUL byte into dest[len].  Thus, the length
   of the destination buffer must be at least LEN + 1.
   The DEST and SRC buffers must not overlap.  */
static inline char *
stzncpy (char *restrict dest, char const *restrict src, size_t len)
{
  size_t i;
  for (i = 0; i < len && *src; i++)
    *dest++ = *src++;
  *dest = 0;
  return dest;
}

/* Return true if ERR is ENOTSUP or EOPNOTSUPP, otherwise false.
   This wrapper function avoids the redundant 'or'd comparison on
   systems like Linux for which they have the same value.  It also
   avoids the gcc warning to that effect.  */
static inline bool
is_ENOTSUP (int err)
{
  return err == EOPNOTSUPP || (ENOTSUP != EOPNOTSUPP && err == ENOTSUP);
}


/* How coreutils quotes filenames, to minimize use of outer quotes,
   but also provide better support for copy and paste when used.  */
#include "quotearg.h"

/* Use these to shell quote only when necessary,
   when the quoted item is already delimited with colons.  */
#define quotef(arg) \
  quotearg_n_style_colon (0, shell_escape_quoting_style, arg)
#define quotef_n(n, arg) \
  quotearg_n_style_colon (n, shell_escape_quoting_style, arg)

/* Use these when there are spaces around the file name,
   in the error message.  */
#define quoteaf(arg) \
  quotearg_style (shell_escape_always_quoting_style, arg)
#define quoteaf_n(n, arg) \
  quotearg_n_style (n, shell_escape_always_quoting_style, arg)

/* Used instead of XARGMATCH() to provide a custom error message.  */
#ifdef XARGMATCH
static inline ptrdiff_t
x_timestyle_match (char const * style, bool allow_posix,
                   char const *const * timestyle_args,
                   char const * timestyle_types,
                   size_t timestyle_types_size,
                   int fail_status)
{
  ptrdiff_t res = argmatch (style, timestyle_args,
                            (char const *) timestyle_types,
                            timestyle_types_size);
  if (res < 0)
    {
      /* This whole block used to be a simple use of XARGMATCH.
         but that didn't print the "posix-"-prefixed variants or
         the "+"-prefixed format string option upon failure.  */
      argmatch_invalid ("time style", style, res);

      /* The following is a manual expansion of argmatch_valid,
         but with the added "+ ..." description and the [posix-]
         prefixes prepended.  Note that this simplification works
         only because all four existing time_style_types values
         are distinct.  */
      fputs (_("Valid arguments are:\n"), stderr);
      char const *const *p = timestyle_args;
      char const *posix_prefix = allow_posix ? "[posix-]" : "";
      while (*p)
        fprintf (stderr, "  - %s%s\n", posix_prefix, *p++);
      fputs (_("  - +FORMAT (e.g., +%H:%M) for a 'date'-style"
               " format\n"), stderr);
      usage (fail_status);
    }

  return res;
}
#endif
