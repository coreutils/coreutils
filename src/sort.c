/* sort - sort lines of text (with all kinds of options).
   Copyright (C) 88, 1991-2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Written December 1988 by Mike Haertel.
   The author may be reached (Email) at the address mike@gnu.ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation.

   Ørn E. Hansen added NLS support in 1997.  */

#include <config.h>

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include "system.h"
#include "closeout.h"
#include "long-options.h"
#include "error.h"
#include "hard-locale.h"
#include "human.h"
#include "memcoll.h"
#include "physmem.h"
#include "xalloc.h"
#include "xstrtol.h"

#if HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifndef RLIMIT_DATA
struct rlimit { size_t rlim_cur; };
# define getrlimit(Resource, Rlp) (-1)
#endif
#ifndef RLIMIT_AS
# define RLIMIT_AS RLIMIT_DATA
#endif

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "sort"

#define AUTHORS "Mike Haertel and Paul Eggert"

#if defined ENABLE_NLS && HAVE_LANGINFO_H
# include <langinfo.h>
#endif

#ifndef SA_NOCLDSTOP
# define sigprocmask(How, Set, Oset) /* empty */
# define sigset_t int
#endif

#ifndef STDC_HEADERS
double strtod ();
#endif

/* Undefine, to avoid warning about redefinition on some systems.  */
/* FIXME: Remove these: use MIN/MAX from sys2.h.  */
#undef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))

#define UCHAR_LIM (UCHAR_MAX + 1)
#define UCHAR(c) ((unsigned char) (c))

#ifndef DEFAULT_TMPDIR
# define DEFAULT_TMPDIR "/tmp"
#endif

/* Use this as exit status in case of error, not EXIT_FAILURE.  This
   is necessary because EXIT_FAILURE is usually 1 and POSIX requires
   that sort exit with status 1 IFF invoked with -c and the input is
   not properly sorted.  Any other irregular exit must exit with a
   status code greater than 1.  */
#define SORT_FAILURE 2
#define SORT_OUT_OF_ORDER 1

#define C_DECIMAL_POINT '.'
#define NEGATION_SIGN   '-'
#define NUMERIC_ZERO    '0'

#ifdef ENABLE_NLS

static char decimal_point;
static int th_sep; /* if CHAR_MAX + 1, then there is no thousands separator */

/* Nonzero if the corresponding locales are hard.  */
static int hard_LC_COLLATE;
# if HAVE_NL_LANGINFO
static int hard_LC_TIME;
# endif

# define IS_THOUSANDS_SEP(x) ((x) == th_sep)

#else

# define decimal_point C_DECIMAL_POINT
# define IS_THOUSANDS_SEP(x) 0

#endif

#define NONZERO(x) (x != 0)

/* The kind of blanks for '-b' to skip in various options. */
enum blanktype { bl_start, bl_end, bl_both };

/* The character marking end of line. Default to \n. */
static int eolchar = '\n';

/* Lines are held in core as counted strings. */
struct line
{
  char *text;			/* Text of the line. */
  size_t length;		/* Length including final newline. */
  char *keybeg;			/* Start of first key. */
  char *keylim;			/* Limit of first key. */
};

/* Input buffers. */
struct buffer
{
  char *buf;			/* Dynamically allocated buffer,
				   partitioned into 3 regions:
				   - input data;
				   - unused area;
				   - an array of lines, in reverse order.  */
  size_t used;			/* Number of bytes used for input data.  */
  size_t nlines;		/* Number of lines in the line array.  */
  size_t alloc;			/* Number of bytes allocated. */
  size_t left;			/* Number of bytes left from previous reads. */
  size_t line_bytes;		/* Number of bytes to reserve for each line. */
  int eof;			/* An EOF has been read.  */
};

struct keyfield
{
  size_t sword;			/* Zero-origin 'word' to start at. */
  size_t schar;			/* Additional characters to skip. */
  int skipsblanks;		/* Skip leading white space at start. */
  size_t eword;			/* Zero-origin first word after field. */
  size_t echar;			/* Additional characters in field. */
  int skipeblanks;		/* Skip trailing white space at finish. */
  int *ignore;			/* Boolean array of characters to ignore. */
  char *translate;		/* Translation applied to characters. */
  int numeric;			/* Flag for numeric comparison.  Handle
				   strings of digits with optional decimal
				   point, but no exponential notation. */
  int general_numeric;		/* Flag for general, numeric comparison.
				   Handle numbers in exponential notation. */
  int month;			/* Flag for comparison by month name. */
  int reverse;			/* Reverse the sense of comparison. */
  struct keyfield *next;	/* Next keyfield to try. */
};

struct month
{
  char *name;
  int val;
};

/* The name this program was run with. */
char *program_name;

/* FIXME: None of these tables work with multibyte character sets.
   Also, there are many other bugs when handling multibyte characters,
   or even unibyte encodings where line boundaries are not in the
   initial shift state.  One way to fix this is to rewrite `sort' to
   use wide characters internally, but doing this with good
   performance is a bit tricky.  */

/* Table of white space. */
static int blanks[UCHAR_LIM];

/* Table of non-printing characters. */
static int nonprinting[UCHAR_LIM];

/* Table of non-dictionary characters (not letters, digits, or blanks). */
static int nondictionary[UCHAR_LIM];

/* Translation table folding lower case to upper.  */
static char fold_toupper[UCHAR_LIM];

#define MONTHS_PER_YEAR 12

#if defined ENABLE_NLS && HAVE_NL_LANGINFO
# define MONTHTAB_CONST /* empty */
#else
# define MONTHTAB_CONST const
#endif

/* Table mapping month names to integers.
   Alphabetic order allows binary search. */
static MONTHTAB_CONST struct month monthtab[] =
{
  {"APR", 4},
  {"AUG", 8},
  {"DEC", 12},
  {"FEB", 2},
  {"JAN", 1},
  {"JUL", 7},
  {"JUN", 6},
  {"MAR", 3},
  {"MAY", 5},
  {"NOV", 11},
  {"OCT", 10},
  {"SEP", 9}
};

/* During the merge phase, the number of files to merge at once. */
#define NMERGE 16

/* The approximate maximum number of bytes of main memory to use.  */
static size_t sort_size;

/* Minimum sort size; the code might not work with smaller sizes.  */
#define MIN_SORT_SIZE (NMERGE * (2 + sizeof (struct line)))

/* The guessed size for non-regular files.  */
#define INPUT_FILE_SIZE_GUESS (1024 * 1024)

/* Array of directory names in which any temporary files are to be created. */
static char const **temp_dirs;

/* Number of temporary directory names used.  */
static size_t temp_dir_count;

/* Number of allocated slots in temp_dirs.  */
static size_t temp_dir_alloc;

/* Our process ID.  */
static pid_t process_id;

/* Flag to reverse the order of all comparisons. */
static int reverse;

/* Flag for stable sort.  This turns off the last ditch bytewise
   comparison of lines, and instead leaves lines in the same order
   they were read if all keys compare equal.  */
static int stable;

/* Tab character separating fields.  If NUL, then fields are separated
   by the empty string between a non-whitespace character and a whitespace
   character. */
static char tab;

/* Flag to remove consecutive duplicate lines from the output.
   Only the last of a sequence of equal lines will be output. */
static int unique;

/* Nonzero if any of the input files are the standard input. */
static int have_read_stdin;

/* List of key field comparisons to be tried.  */
static struct keyfield *keylist;

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
"),
	      program_name);
      printf (_("\
Write sorted concatenation of all FILE(s) to standard output.\n\
\n\
  +POS1 [-POS2]    start a key at POS1, end it *before* POS2 (obsolescent)\n\
                     field numbers and character offsets are numbered\n\
                     starting with zero (contrast with the -k option)\n\
  -b               ignore leading blanks in sort fields or keys\n\
  -c               check if given files already sorted, do not sort\n\
  -d               consider only blanks and alphanumeric characters in keys\n\
  -f               fold lower case to upper case characters in keys\n\
  -g               compare according to general numerical value, imply -b\n\
  -i               consider only printable characters in keys\n\
  -k POS1[,POS2]   start a key at POS1, end it *at* POS2\n\
                     field numbers and character offsets are numbered\n\
                     starting with one (contrast with zero-based +POS form)\n\
  -m               merge already sorted files, do not sort\n\
  -M               compare (unknown) < `JAN' < ... < `DEC', imply -b\n\
  -n               compare according to string numerical value, imply -b\n\
  -o FILE          write result on FILE instead of standard output\n\
  -r               reverse the result of comparisons\n\
  -s               stabilize sort by disabling last resort comparison\n\
  -S SIZE          use SIZE for main memory sorting\n\
  -t SEP           use SEParator instead of non- to whitespace transition\n\
  -T DIRECTORY     use DIRECTORY for temporary files, not $TMPDIR or %s\n\
                     multiple -T options specify multiple directories\n\
  -u               with -c, check for strict ordering;\n\
                   with -m, only output the first of an equal sequence\n\
  -z               end lines with 0 byte, not newline, for find -print0\n\
      --help       display this help and exit\n\
      --version    output version information and exit\n\
\n\
"),
	      DEFAULT_TMPDIR);
      printf (_("\
POS is F[.C][OPTS], where F is the field number and C the character position\n\
in the field, both counted from one with -k, from zero with the obsolescent\n\
form.  OPTS is made up of one or more of Mbdfinr; this effectively disables\n\
global -Mbdfinr settings for that key.  If no key is given, use the entire\n\
line as the key.\n\
\n\
SIZE may be followed by the following multiplicative suffixes:\n\
%% 1%% of memory, b 1, k 1024 (default), and so on for M, G, T, P, E, Z, Y.\n\
\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
*** WARNING ***\n\
The locale specified by the environment affects sort order.\n\
Set LC_ALL=C to get the traditional sort order that uses native byte values.\n\
")
	      );
      puts (_("\nReport bugs to <bug-textutils@gnu.org>."));
    }
  /* Don't use EXIT_FAILURE here in case it is defined to be 1.
     POSIX requires that sort return 1 IFF invoked with -c and
     the input is not properly sorted.  */
  assert (status == 0 || status == SORT_FAILURE);
  exit (status);
}

/* The set of signals that are caught.  */
static sigset_t caught_signals;

/* The list of temporary files. */
struct tempnode
{
  struct tempnode *volatile next;
  char name[1];  /* Actual size is 1 + file name length.  */
};
static struct tempnode *volatile temphead;

/* Clean up any remaining temporary files. */

static void
cleanup (void)
{
  struct tempnode *node;

  for (node = temphead; node; node = node->next)
    unlink (node->name);
}

/* Create a new temporary file, returning its newly allocated name.
   Store into *PFP a stream open for writing.  */

static char *
create_temp_file (FILE **pfp)
{
  static char const slashbase[] = "/sortXXXXXX";
  static size_t temp_dir_index;
  sigset_t oldset;
  int fd;
  int saved_errno;
  char const *temp_dir = temp_dirs[temp_dir_index];
  size_t len = strlen (temp_dir);
  struct tempnode *node =
    (struct tempnode *) xmalloc (sizeof node->next + len + sizeof slashbase);
  char *file = node->name;

  memcpy (file, temp_dir, len);
  memcpy (file + len, slashbase, sizeof slashbase);
  node->next = temphead;
  if (++temp_dir_index == temp_dir_count)
    temp_dir_index = 0;

  /* Create the temporary file in a critical section, to avoid races.  */
  sigprocmask (SIG_BLOCK, &caught_signals, &oldset);
  fd = mkstemp (file);
  if (0 <= fd)
    temphead = node;
  saved_errno = errno;
  sigprocmask (SIG_SETMASK, &oldset, NULL);
  errno = saved_errno;

  if (fd < 0 || (*pfp = fdopen (fd, "w")) == NULL)
    {
      error (0, errno, "%s", file);
      cleanup ();
      exit (SORT_FAILURE);
    }

  return file;
}

static FILE *
xfopen (const char *file, const char *how)
{
  FILE *fp;

  if (STREQ (file, "-"))
    {
      fp = stdin;
    }
  else
    {
      if ((fp = fopen (file, how)) == NULL)
	{
	  error (0, errno, "%s", file);
	  cleanup ();
	  exit (SORT_FAILURE);
	}
    }

  if (fp == stdin)
    have_read_stdin = 1;
  return fp;
}

static void
xfclose (FILE *fp)
{
  if (fp == stdin)
    {
      /* Allow reading stdin from tty more than once. */
      if (feof (fp))
	clearerr (fp);
    }
  else if (fp == stdout)
    {
      if (fflush (fp) != 0)
	{
	  error (0, errno, _("flushing file"));
	  cleanup ();
	  exit (SORT_FAILURE);
	}
    }
  else
    {
      if (fclose (fp) != 0)
	{
	  error (0, errno, _("error closing file"));
	  cleanup ();
	  exit (SORT_FAILURE);
	}
    }
}

static void
write_bytes (const char *buf, size_t n_bytes, FILE *fp, const char *output_file)
{
  if (fwrite (buf, 1, n_bytes, fp) != n_bytes)
    {
      error (0, errno, _("%s: write error"), output_file);
      cleanup ();
      exit (SORT_FAILURE);
    }
}

/* Append DIR to the array of temporary directory names.  */
static void
add_temp_dir (char const *dir)
{
  if (temp_dir_count == temp_dir_alloc)
    {
      temp_dir_alloc = temp_dir_alloc ? temp_dir_alloc * 2 : 16;
      temp_dirs = xrealloc (temp_dirs, sizeof (temp_dirs) * temp_dir_alloc);
    }

  temp_dirs[temp_dir_count++] = dir;
}

/* Search through the list of temporary files for NAME;
   remove it if it is found on the list. */

static void
zaptemp (const char *name)
{
  struct tempnode *volatile *pnode;
  struct tempnode *node;

  for (pnode = &temphead; (node = *pnode); pnode = &node->next)
    if (node->name == name)
      {
	unlink (name);
	*pnode = node->next;
	free ((char *) node);
	break;
      }
}

#ifdef ENABLE_NLS

static int
struct_month_cmp (const void *m1, const void *m2)
{
  return strcmp (((const struct month *) m1)->name,
		 ((const struct month *) m2)->name);
}

#endif /* NLS */

/* Initialize the character class tables. */

static void
inittables (void)
{
  int i;

  for (i = 0; i < UCHAR_LIM; ++i)
    {
      if (ISBLANK (i))
	blanks[i] = 1;
      if (!ISPRINT (i))
	nonprinting[i] = 1;
      if (!ISALNUM (i) && !ISBLANK (i))
	nondictionary[i] = 1;
      if (ISLOWER (i))
	fold_toupper[i] = toupper (i);
      else
	fold_toupper[i] = i;
    }

#if defined ENABLE_NLS && HAVE_NL_LANGINFO
  /* If we're not in the "C" locale, read different names for months.  */
  if (hard_LC_TIME)
    {
      for (i = 0; i < MONTHS_PER_YEAR; i++)
	{
	  char *s;
	  size_t s_len;
	  size_t j;
	  char *name;

	  s = (char *) nl_langinfo (ABMON_1 + i);
	  s_len = strlen (s);
	  monthtab[i].name = name = (char *) xmalloc (s_len + 1);
	  monthtab[i].val = i + 1;

	  for (j = 0; j < s_len; j++)
	    name[j] = fold_toupper[UCHAR (s[j])];
	  name[j] = '\0';
	}
      qsort ((void *) monthtab, MONTHS_PER_YEAR,
	     sizeof (struct month), struct_month_cmp);
    }
#endif /* NLS */
}

/* Specify the amount of main memory to use when sorting.  */
static void
specify_sort_size (char const *s)
{
  uintmax_t n;
  char *suffix;
  enum strtol_error e = xstrtoumax (s, &suffix, 10, &n, "EgGkmMPtTYZ");

  /* The default unit is kB.  */
  if (e == LONGINT_OK && ISDIGIT (suffix[-1]))
    {
      if (n <= UINTMAX_MAX / 1024)
	n *= 1024;
      else
	e = LONGINT_OVERFLOW;
    }

  /* A 'b' suffix means bytes; a '%' suffix means percent of memory.  */
  if (e == LONGINT_INVALID_SUFFIX_CHAR && ISDIGIT (suffix[-1]) && ! suffix[1])
    switch (suffix[0])
      {
      case 'b':
	e = LONGINT_OK;
	break;

      case '%':
	{
	  double mem = physmem_total () * n / 100;

	  /* Use "<", not "<=", to avoid problems with rounding.  */
	  if (mem < UINTMAX_MAX)
	    {
	      n = mem;
	      e = LONGINT_OK;
	    }
	  else
	    e = LONGINT_OVERFLOW;
	}
	break;
      }

  if (e == LONGINT_OK)
    {
      sort_size = n;
      if (sort_size == n)
	{
	  sort_size = MAX (sort_size, MIN_SORT_SIZE);
	  return;
	}

      e = LONGINT_OVERFLOW;
    }

  STRTOL_FATAL_ERROR (s, _("sort size"), e);
}

/* Return the default sort size.  */
static size_t
default_sort_size (void)
{
  /* Let MEM be available memory or 1/8 of total memory, whichever
     is greater.  */
  double avail = physmem_available ();
  double total = physmem_total ();
  double mem = MAX (avail, total / 8);
  struct rlimit rlimit;

  /* Let SIZE be MEM, but no more than the maximum object size or
     system resource limits.  Avoid the MIN macro here, as it is not
     quite right when only one argument is floating point.  Don't
     bother to check for values like RLIM_INFINITY since in practice
     they are not much less than SIZE_MAX.  */
  size_t size = SIZE_MAX;
  if (mem < size)
    size = mem;
  if (getrlimit (RLIMIT_DATA, &rlimit) == 0 && rlimit.rlim_cur < size)
    size = rlimit.rlim_cur;
  if (getrlimit (RLIMIT_AS, &rlimit) == 0 && rlimit.rlim_cur < size)
    size = rlimit.rlim_cur;

  /* Use half of SIZE, but no less than the minimum.  */
  return MAX (size / 2, MIN_SORT_SIZE);
}

/* Return the sort buffer size to use with the input files identified
   by FPS and FILES, which are alternate paths to the same files.
   NFILES gives the number of input files; NFPS may be less.  Assume
   that each input line requires LINE_BYTES extra bytes' worth of line
   information.  Return at most SIZE_BOUND.  */

static size_t
sort_buffer_size (FILE *const *fps, int nfps,
		  char *const *files, int nfiles,
		  size_t line_bytes, size_t size_bound)
{
  /* In the worst case, each input byte is a newline.  */
  size_t worst_case_per_input_byte = line_bytes + 1;

  /* Keep enough room for one extra input line and an extra byte.
     This extra room might be needed when preparing to read EOF.  */
  size_t size = worst_case_per_input_byte + 1;

  int i;

  for (i = 0; i < nfiles; i++)
    {
      struct stat st;
      off_t file_size;
      size_t worst_case;

      if ((i < nfps ? fstat (fileno (fps[i]), &st)
	   : strcmp (files[i], "-") == 0 ? fstat (STDIN_FILENO, &st)
	   : stat (files[i], &st))
	  != 0)
	{
	  error (0, errno, "%s", files[i]);
	  cleanup ();
	  exit (SORT_FAILURE);
	}

      file_size = S_ISREG (st.st_mode) ? st.st_size : INPUT_FILE_SIZE_GUESS;

      /* Add the amount of memory needed to represent the worst case
	 where the input consists entirely of newlines followed by a
	 single non-newline.  Check for overflow.  */
      worst_case = file_size * worst_case_per_input_byte + 1;
      if (file_size != worst_case / worst_case_per_input_byte
	  || size_bound - size <= worst_case)
	return size_bound;
      size += worst_case;
    }

  return size;
}

/* Initialize BUF.  Reserve LINE_BYTES bytes for each line; LINE_BYTES
   must be at least sizeof (struct line).  Allocate ALLOC bytes
   initially.  */

static void
initbuf (struct buffer *buf, size_t line_bytes, size_t alloc)
{
  /* Ensure that the line array is properly aligned.  If the desired
     size cannot be allocated, repeatedly halve it until allocation
     succeeds.  The smaller allocation may hurt overall performance,
     but that's better than failing.  */
  for (;;)
    {
      alloc += sizeof (struct line) - alloc % sizeof (struct line);
      buf->buf = malloc (alloc);
      if (buf->buf)
	break;
      alloc /= 2;
      if (alloc <= line_bytes + 1)
	xalloc_die ();
    }

  buf->line_bytes = line_bytes;
  buf->alloc = alloc;
  buf->used = buf->left = buf->nlines = 0;
  buf->eof = 0;
}

/* Return one past the limit of the line array.  */

static inline struct line *
buffer_linelim (struct buffer const *buf)
{
  return (struct line *) (buf->buf + buf->alloc);
}

/* Return a pointer to the first character of the field specified
   by KEY in LINE. */

static char *
begfield (const struct line *line, const struct keyfield *key)
{
  register char *ptr = line->text, *lim = ptr + line->length - 1;
  register size_t sword = key->sword;
  register size_t schar = key->schar;

  if (tab)
    while (ptr < lim && sword--)
      {
	while (ptr < lim && *ptr != tab)
	  ++ptr;
	if (ptr < lim)
	  ++ptr;
      }
  else
    while (ptr < lim && sword--)
      {
	while (ptr < lim && blanks[UCHAR (*ptr)])
	  ++ptr;
	while (ptr < lim && !blanks[UCHAR (*ptr)])
	  ++ptr;
      }

  if (key->skipsblanks)
    while (ptr < lim && blanks[UCHAR (*ptr)])
      ++ptr;

  if (schar < lim - ptr)
    ptr += schar;
  else
    ptr = lim;

  return ptr;
}

/* Return the limit of (a pointer to the first character after) the field
   in LINE specified by KEY. */

static char *
limfield (const struct line *line, const struct keyfield *key)
{
  register char *ptr = line->text, *lim = ptr + line->length - 1;
  register size_t eword = key->eword, echar = key->echar;

  /* Note: from the POSIX spec:
     The leading field separator itself is included in
     a field when -t is not used.  FIXME: move this comment up... */

  /* Move PTR past EWORD fields or to one past the last byte on LINE,
     whichever comes first.  If there are more than EWORD fields, leave
     PTR pointing at the beginning of the field having zero-based index,
     EWORD.  If a delimiter character was specified (via -t), then that
     `beginning' is the first character following the delimiting TAB.
     Otherwise, leave PTR pointing at the first `blank' character after
     the preceding field.  */
  if (tab)
    while (ptr < lim && eword--)
      {
	while (ptr < lim && *ptr != tab)
	  ++ptr;
	if (ptr < lim && (eword | echar))
	  ++ptr;
      }
  else
    while (ptr < lim && eword--)
      {
	while (ptr < lim && blanks[UCHAR (*ptr)])
	  ++ptr;
	while (ptr < lim && !blanks[UCHAR (*ptr)])
	  ++ptr;
      }

#ifdef POSIX_UNSPECIFIED
  /* The following block of code makes GNU sort incompatible with
     standard Unix sort, so it's ifdef'd out for now.
     The POSIX spec isn't clear on how to interpret this.
     FIXME: request clarification.

     From: kwzh@gnu.ai.mit.edu (Karl Heuer)
     Date: Thu, 30 May 96 12:20:41 -0400

     [...]I believe I've found another bug in `sort'.

     $ cat /tmp/sort.in
     a b c 2 d
     pq rs 1 t
     $ textutils-1.15/src/sort +0.6 -0.7 </tmp/sort.in
     a b c 2 d
     pq rs 1 t
     $ /bin/sort +0.6 -0.7 </tmp/sort.in
     pq rs 1 t
     a b c 2 d

     Unix sort produced the answer I expected: sort on the single character
     in column 6.  GNU sort produced different results, because it disagrees
     on the interpretation of the key-end spec "-M.N".  Unix sort reads this
     as "skip M fields, then N characters"; but GNU sort wants it to mean
     "skip M fields, then either N characters or the rest of the current
     field, whichever comes first".  This extra clause applies only to
     key-ends, not key-starts.
     */

  /* Make LIM point to the end of (one byte past) the current field.  */
  if (tab)
    {
      char *newlim;
      newlim = memchr (ptr, tab, lim - ptr);
      if (newlim)
	lim = newlim;
    }
  else
    {
      char *newlim;
      newlim = ptr;
      while (newlim < lim && blanks[UCHAR (*newlim)])
	++newlim;
      while (newlim < lim && !blanks[UCHAR (*newlim)])
	++newlim;
      lim = newlim;
    }
#endif

  /* If we're skipping leading blanks, don't start counting characters
     until after skipping past any leading blanks.  */
  if (key->skipsblanks)
    while (ptr < lim && blanks[UCHAR (*ptr)])
      ++ptr;

  /* Advance PTR by ECHAR (if possible), but no further than LIM.  */
  if (echar < lim - ptr)
    ptr += echar;
  else
    ptr = lim;

  return ptr;
}

/* FIXME */

static void
trim_trailing_blanks (const char *a_start, char **a_end)
{
  while (*a_end > a_start && blanks[UCHAR (*(*a_end - 1))])
    --(*a_end);
}

/* Fill BUF reading from FP, moving buf->left bytes from the end
   of buf->buf to the beginning first.  If EOF is reached and the
   file wasn't terminated by a newline, supply one.  Set up BUF's line
   table too.  FILE is the name of the file corresponding to FP.
   Return nonzero if some input was read.  */

static int
fillbuf (struct buffer *buf, register FILE *fp, char const *file)
{
  struct keyfield const *key = keylist;
  char eol = eolchar;
  size_t line_bytes = buf->line_bytes;

  if (buf->eof)
    return 0;

  if (buf->used != buf->left)
    {
      memmove (buf->buf, buf->buf + buf->used - buf->left, buf->left);
      buf->used = buf->left;
      buf->nlines = 0;
    }

  for (;;)
    {
      char *ptr = buf->buf + buf->used;
      struct line *linelim = buffer_linelim (buf);
      struct line *line = linelim - buf->nlines;
      size_t avail = (char *) linelim - buf->nlines * line_bytes - ptr;
      char *line_start = buf->nlines ? line->text + line->length : buf->buf;

      while (line_bytes + 1 < avail)
	{
	  /* Read as many bytes as possible, but do not read so many
	     bytes that there might not be enough room for the
	     corresponding line array.  The worst case is when the
	     rest of the input file consists entirely of newlines,
	     except that the last byte is not a newline.  */
	  size_t readsize = (avail - 1) / (line_bytes + 1);
	  size_t bytes_read = fread (ptr, 1, readsize, fp);
	  char *ptrlim = ptr + bytes_read;
	  char *p;
	  avail -= bytes_read;

	  if (bytes_read != readsize)
	    {
	      if (ferror (fp))
		{
		  error (0, errno, "%s", file);
		  cleanup ();
		  exit (SORT_FAILURE);
		}
	      if (feof (fp))
		{
		  buf->eof = 1;
		  if (buf->buf == ptrlim)
		    return 0;
		  if (ptrlim[-1] != eol)
		    *ptrlim++ = eol;
		}
	    }

	  /* Find and record each line in the just-read input.  */
	  while ((p = memchr (ptr, eol, ptrlim - ptr)))
	    {
	      ptr = p + 1;
	      line--;
	      line->text = line_start;
	      line->length = ptr - line_start;
	      avail -= line_bytes;

	      if (key)
		{
		  /* Precompute the position of the first key for
                     efficiency. */
		  line->keylim = key->eword == -1 ? p : limfield (line, key);

		  if (key->sword != -1)
		    line->keybeg = begfield (line, key);
		  else
		    {
		      if (key->skipsblanks)
			while (blanks[UCHAR (*line_start)])
			  line_start++;
		      line->keybeg = line_start;
		    }
		  if (key->skipeblanks)
		    trim_trailing_blanks (line->keybeg, &line->keylim);
		}

	      line_start = ptr;
	    }

	  ptr = ptrlim;
	  if (buf->eof)
	    break;
	}

      buf->used = ptr - buf->buf;
      buf->nlines = buffer_linelim (buf) - line;
      if (buf->nlines != 0)
	{
	  buf->left = ptr - line_start;
	  return 1;
	}

      /* The current input line is too long to fit in the buffer.
	 Double the buffer size and try again.  */
      if (2 * buf->alloc < buf->alloc)
	xalloc_die ();
      buf->alloc *= 2;
      buf->buf = xrealloc (buf->buf, buf->alloc);
    }
}

/* Compare strings A and B containing decimal fractions < 1.  Each string
   should begin with a decimal point followed immediately by the digits
   of the fraction.  Strings not of this form are considered to be zero. */

/* The goal here, is to take two numbers a and b... compare these
   in parallel.  Instead of converting each, and then comparing the
   outcome.  Most likely stopping the comparison before the conversion
   is complete.  The algorithm used, in the old sort:

   Algorithm: fraccompare
   Action   : compare two decimal fractions
   accepts  : char *a, char *b
   returns  : -1 if a<b, 0 if a=b, 1 if a>b.
   implement:

   if *a == decimal_point AND *b == decimal_point
     find first character different in a and b.
     if both are digits, return the difference *a - *b.
     if *a is a digit
       skip past zeros
       if digit return 1, else 0
     if *b is a digit
       skip past zeros
       if digit return -1, else 0
   if *a is a decimal_point
     skip past decimal_point and zeros
     if digit return 1, else 0
   if *b is a decimal_point
     skip past decimal_point and zeros
     if digit return -1, else 0
   return 0 */

static int
fraccompare (register const char *a, register const char *b)
{
  if (*a == decimal_point && *b == decimal_point)
    {
      while (*++a == *++b)
	if (! ISDIGIT (*a))
	  return 0;
      if (ISDIGIT (*a) && ISDIGIT (*b))
	return *a - *b;
      if (ISDIGIT (*a))
	goto a_trailing_nonzero;
      if (ISDIGIT (*b))
	goto b_trailing_nonzero;
      return 0;
    }
  else if (*a++ == decimal_point)
    {
    a_trailing_nonzero:
      while (*a == NUMERIC_ZERO)
	a++;
      return ISDIGIT (*a);
    }
  else if (*b++ == decimal_point)
    {
    b_trailing_nonzero:
      while (*b == NUMERIC_ZERO)
	b++;
      return - ISDIGIT (*b);
    }
  return 0;
}

/* Compare strings A and B as numbers without explicitly converting them to
   machine numbers.  Comparatively slow for short strings, but asymptotically
   hideously fast. */

static int
numcompare (register const char *a, register const char *b)
{
  register int tmpa, tmpb, tmp;
  register size_t loga, logb;

  tmpa = *a;
  tmpb = *b;

  while (blanks[UCHAR (tmpa)])
    tmpa = *++a;
  while (blanks[UCHAR (tmpb)])
    tmpb = *++b;

  if (tmpa == NEGATION_SIGN)
    {
      do
	tmpa = *++a;
      while (tmpa == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpa));
      if (tmpb != NEGATION_SIGN)
	{
	  if (tmpa == decimal_point)
	    do
	      tmpa = *++a;
	    while (tmpa == NUMERIC_ZERO);
	  if (ISDIGIT (tmpa))
	    return -1;
	  while (tmpb == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpb))
	    tmpb = *++b;
	  if (tmpb == decimal_point)
	    do
	      tmpb = *++b;
	    while (tmpb == NUMERIC_ZERO);
	  if (ISDIGIT (tmpb))
	    return -1;
	  return 0;
	}
      do
	tmpb = *++b;
      while (tmpb == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpb));

      while (tmpa == tmpb && ISDIGIT (tmpa))
	{
	  do
	    tmpa = *++a;
	  while (IS_THOUSANDS_SEP (tmpa));
	  do
	    tmpb = *++b;
	  while (IS_THOUSANDS_SEP (tmpb));
	}

      if ((tmpa == decimal_point && !ISDIGIT (tmpb))
	  || (tmpb == decimal_point && !ISDIGIT (tmpa)))
	return -fraccompare (a, b);

      tmp = tmpb - tmpa;

      for (loga = 0; ISDIGIT (tmpa); ++loga)
	do
	  tmpa = *++a;
	while (IS_THOUSANDS_SEP (tmpa));

      for (logb = 0; ISDIGIT (tmpb); ++logb)
	do
	  tmpb = *++b;
	while (IS_THOUSANDS_SEP (tmpb));

      if (loga != logb)
	return loga < logb ? 1 : -1;

      if (!loga)
	return 0;

      return tmp;
    }
  else if (tmpb == NEGATION_SIGN)
    {
      do
	tmpb = *++b;
      while (tmpb == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpb));
      if (tmpb == decimal_point)
	do
	  tmpb = *++b;
	while (tmpb == NUMERIC_ZERO);
      if (ISDIGIT (tmpb))
	return 1;
      while (tmpa == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpa))
	tmpa = *++a;
      if (tmpa == decimal_point)
	do
	  tmpa = *++a;
	while (tmpa == NUMERIC_ZERO);
      if (ISDIGIT (tmpa))
	return 1;
      return 0;
    }
  else
    {
      while (tmpa == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpa))
	tmpa = *++a;
      while (tmpb == NUMERIC_ZERO || IS_THOUSANDS_SEP (tmpb))
	tmpb = *++b;

      while (tmpa == tmpb && ISDIGIT (tmpa))
	{
	  do
	    tmpa = *++a;
	  while (IS_THOUSANDS_SEP (tmpa));
	  do
	    tmpb = *++b;
	  while (IS_THOUSANDS_SEP (tmpb));
	}

      if ((tmpa == decimal_point && !ISDIGIT (tmpb))
	  || (tmpb == decimal_point && !ISDIGIT (tmpa)))
	return fraccompare (a, b);

      tmp = tmpa - tmpb;

      for (loga = 0; ISDIGIT (tmpa); ++loga)
	do
	  tmpa = *++a;
	while (IS_THOUSANDS_SEP (tmpa));

      for (logb = 0; ISDIGIT (tmpb); ++logb)
	do
	  tmpb = *++b;
	while (IS_THOUSANDS_SEP (tmpb));

      if (loga != logb)
	return loga < logb ? -1 : 1;

      if (!loga)
	return 0;

      return tmp;
    }
}

static int
general_numcompare (const char *sa, const char *sb)
{
  /* FIXME: add option to warn about failed conversions.  */
  /* FIXME: maybe add option to try expensive FP conversion
     only if A and B can't be compared more cheaply/accurately.  */

  char *ea;
  char *eb;
  double a = strtod (sa, &ea);
  double b = strtod (sb, &eb);

  /* Put conversion errors at the start of the collating sequence.  */
  if (sa == ea)
    return sb == eb ? 0 : -1;
  if (sb == eb)
    return 1;

  /* Sort numbers in the usual way, where -0 == +0.  Put NaNs after
     conversion errors but before numbers; sort them by internal
     bit-pattern, for lack of a more portable alternative.  */
  return (a < b ? -1
	  : a > b ? 1
	  : a == b ? 0
	  : b == b ? -1
	  : a == a ? 1
	  : memcmp ((char *) &a, (char *) &b, sizeof a));
}

/* Return an integer in 1..12 of the month name S with length LEN.
   Return 0 if the name in S is not recognized.  */

static int
getmonth (const char *s, size_t len)
{
  char *month;
  register size_t i;
  register int lo = 0, hi = MONTHS_PER_YEAR, result;

  while (len > 0 && blanks[UCHAR (*s)])
    {
      ++s;
      --len;
    }

  if (len == 0)
    return 0;

  month = (char *) alloca (len + 1);
  for (i = 0; i < len; ++i)
    month[i] = fold_toupper[UCHAR (s[i])];
  while (blanks[UCHAR (month[i - 1])])
    --i;
  month[i] = '\0';

  do
    {
      int ix = (lo + hi) / 2;

      if (strncmp (month, monthtab[ix].name, strlen (monthtab[ix].name)) < 0)
	hi = ix;
      else
	lo = ix;
    }
  while (hi - lo > 1);

  result = (!strncmp (month, monthtab[lo].name, strlen (monthtab[lo].name))
	    ? monthtab[lo].val : 0);

  return result;
}

/* Compare two lines A and B trying every key in sequence until there
   are no more keys or a difference is found. */

static int
keycompare (const struct line *a, const struct line *b)
{
  struct keyfield *key = keylist;

  /* For the first iteration only, the key positions have been
     precomputed for us. */
  register char *texta = a->keybeg;
  register char *textb = b->keybeg;
  register char *lima = a->keylim;
  register char *limb = b->keylim;

  int diff;

  for (;;)
    {
      register unsigned char *translate = (unsigned char *) key->translate;
      register int *ignore = key->ignore;

      /* Find the lengths. */
      size_t lena = lima <= texta ? 0 : lima - texta;
      size_t lenb = limb <= textb ? 0 : limb - textb;

      if (key->skipeblanks)
	{
	  char *a_end = texta + lena;
	  char *b_end = textb + lenb;
	  trim_trailing_blanks (texta, &a_end);
	  trim_trailing_blanks (textb, &b_end);
	  lena = a_end - texta;
	  lenb = b_end - textb;
	}

      /* Actually compare the fields. */
      if (key->numeric | key->general_numeric)
	{
	  char savea = *lima, saveb = *limb;

	  *lima = *limb = '\0';
	  diff = ((key->numeric ? numcompare : general_numcompare)
		  (texta, textb));
	  *lima = savea, *limb = saveb;
	}
      else if (key->month)
	diff = getmonth (texta, lena) - getmonth (textb, lenb);
#ifdef ENABLE_NLS
      /* Sorting like this may become slow, so in a simple locale the user
         can select a faster sort that is similar to ascii sort  */
      else if (hard_LC_COLLATE)
	{
	  if (ignore || translate)
	    {
	      char *copy_a = (char *) alloca (lena + 1 + lenb + 1);
	      char *copy_b = copy_a + lena + 1;
	      size_t new_len_a, new_len_b, i;

	      /* Ignore and/or translate chars before comparing.  */
	      for (new_len_a = new_len_b = i = 0; i < max (lena, lenb); i++)
		{
		  if (i < lena)
		    {
		      copy_a[new_len_a] = (translate
					   ? translate[UCHAR (texta[i])]
					   : texta[i]);
		      if (!ignore || !ignore[UCHAR (texta[i])])
			++new_len_a;
		    }
		  if (i < lenb)
		    {
		      copy_b[new_len_b] = (translate
					   ? translate[UCHAR (textb[i])]
					   : textb [i]);
		      if (!ignore || !ignore[UCHAR (textb[i])])
			++new_len_b;
		    }
		}

	      diff = memcoll (copy_a, new_len_a, copy_b, new_len_b);
	    }
	  else if (lena == 0)
	    diff = - NONZERO (lenb);
	  else if (lenb == 0)
	    goto greater;
	  else
	    diff = memcoll (texta, lena, textb, lenb);
	}
#endif
      else if (ignore)
	{
#define CMP_WITH_IGNORE(A, B)						\
  do									\
    {									\
	  for (;;)							\
	    {								\
	      while (texta < lima && ignore[UCHAR (*texta)])		\
		++texta;						\
	      while (textb < limb && ignore[UCHAR (*textb)])		\
		++textb;						\
	      if (! (texta < lima && textb < limb))			\
		break;							\
	      diff = UCHAR (A) - UCHAR (B);				\
	      if (diff)							\
		goto not_equal;						\
	      ++texta;							\
	      ++textb;							\
	    }								\
									\
	  diff = (lima - texta) - (limb - textb);			\
    }									\
  while (0)

	  if (translate)
	    CMP_WITH_IGNORE (translate[UCHAR (*texta)],
			     translate[UCHAR (*textb)]);
	  else
	    CMP_WITH_IGNORE (UCHAR (*texta), UCHAR (*textb));
	}
      else if (lena == 0)
	diff = - NONZERO (lenb);
      else if (lenb == 0)
	goto greater;
      else
	{
	  if (translate)
	    {
	      while (texta < lima && textb < limb)
		{
		  diff = (UCHAR (translate[UCHAR (*texta++)])
			  - UCHAR (translate[UCHAR (*textb++)]));
		  if (diff)
		    goto not_equal;
		}
	    }
	  else
	    {
	      diff = memcmp (texta, textb, min (lena, lenb));
	      if (diff)
		goto not_equal;
	    }
	  diff = lena < lenb ? -1 : lena != lenb;
	}

      if (diff)
	goto not_equal;

      key = key->next;
      if (! key)
	break;

      /* Find the beginning and limit of the next field.  */
      if (key->eword != -1)
	lima = limfield (a, key), limb = limfield (b, key);
      else
	lima = a->text + a->length - 1, limb = b->text + b->length - 1;

      if (key->sword != -1)
	texta = begfield (a, key), textb = begfield (b, key);
      else
	{
	  texta = a->text, textb = b->text;
	  if (key->skipsblanks)
	    {
	      while (texta < lima && blanks[UCHAR (*texta)])
		++texta;
	      while (textb < limb && blanks[UCHAR (*textb)])
		++textb;
	    }
	}
    }

  return 0;

 greater:
  diff = 1;
 not_equal:
  return key->reverse ? -diff : diff;
}

/* Compare two lines A and B, returning negative, zero, or positive
   depending on whether A compares less than, equal to, or greater than B. */

static int
compare (register const struct line *a, register const struct line *b)
{
  int diff;
  size_t alen, blen;

  /* First try to compare on the specified keys (if any).
     The only two cases with no key at all are unadorned sort,
     and unadorned sort -r. */
  if (keylist)
    {
      diff = keycompare (a, b);
      alloca (0);
      if (diff != 0 || unique || stable)
	return diff;
    }

  /* If the keys all compare equal (or no keys were specified)
     fall through to the default comparison.  */
  alen = a->length - 1, blen = b->length - 1;

  if (alen == 0)
    diff = - NONZERO (blen);
  else if (blen == 0)
    diff = NONZERO (alen);
#ifdef ENABLE_NLS
  else if (hard_LC_COLLATE)
    diff = memcoll (a->text, alen, b->text, blen);
#endif
  else if (! (diff = memcmp (a->text, b->text, min (alen, blen))))
    diff = alen < blen ? -1 : alen != blen;

  return reverse ? -diff : diff;
}

/* Check that the lines read from the given FP come in order.  Print a
   diagnostic (FILE_NAME, line number, contents of line) to stderr and return
   one if they are not in order.  Otherwise, print no diagnostic
   and return zero.  */

static int
checkfp (FILE *fp, char *file_name)
{
  struct buffer buf;		/* Input buffer. */
  struct line temp;		/* Copy of previous line. */
  size_t alloc = 0;
  uintmax_t line_number = 0;
  struct keyfield *key = keylist;
  int nonunique = 1 - unique;
  int disordered = 0;

  initbuf (&buf, sizeof (struct line),
	   sort_buffer_size (&fp, 1, &file_name, 1,
			     sizeof (struct line), sort_size));
  temp.text = NULL;

  while (fillbuf (&buf, fp, file_name))
    {
      struct line const *line = buffer_linelim (&buf);
      struct line const *linebase = line - buf.nlines;

      /* Make sure the line saved from the old buffer contents is
	 less than or equal to the first line of the new buffer. */
      if (alloc && nonunique <= compare (&temp, line - 1))
	{
	found_disorder:
	  {
	    char hr_buf[LONGEST_HUMAN_READABLE + 1];
	    struct line const *disorder_line = line - 1;
	    uintmax_t disorder_line_number =
	      buffer_linelim (&buf) - disorder_line + line_number;
	    fprintf (stderr, _("%s: %s:%s: disorder: "),
		     program_name, file_name,
		     human_readable (disorder_line_number, hr_buf, 1, 1));
	    write_bytes (disorder_line->text, disorder_line->length, stderr,
			 _("standard error"));
	    disordered = 1;
	    break;
	  }
	}

      /* Compare each line in the buffer with its successor.  */
      while (linebase < --line)
	if (nonunique <= compare (line, line - 1))
	  goto found_disorder;

      line_number += buf.nlines;

      /* Save the last line of the buffer.  */
      if (alloc < line->length)
	{
	  do
	    {
	      alloc *= 2;
	      if (! alloc)
		{
		  alloc = line->length;
		  break;
		}
	    }
	  while (alloc < line->length);

	  temp.text = xrealloc (temp.text, alloc);
	}
      memcpy (temp.text, line->text, line->length);
      temp.length = line->length;
      if (key)
	{
	  temp.keybeg = temp.text + (line->keybeg - line->text);
	  temp.keylim = temp.text + (line->keylim - line->text);
	}
    }

  xfclose (fp);
  free (buf.buf);
  if (temp.text)
    free (temp.text);
  return disordered;
}

/* Merge lines from FPS onto OFP.  NFPS cannot be greater than NMERGE.
   Close FPS before returning.  NAMES and OUTPUT_NAME give the names
   of the files.  */

static void
mergefps (FILE **fps, char **files, register int nfps,
	  FILE *ofp, const char *output_file)
{
  struct buffer buffer[NMERGE];	/* Input buffers for each file. */
  struct line saved;		/* Saved line storage for unique check. */
  struct line const *savedline = NULL;
				/* &saved if there is a saved line. */
  size_t savealloc = 0;		/* Size allocated for the saved line. */
  struct line const *cur[NMERGE]; /* Current line in each line table. */
  struct line const *base[NMERGE]; /* Base of each line table.  */
  int ord[NMERGE];		/* Table representing a permutation of fps,
				   such that cur[ord[0]] is the smallest line
				   and will be next output. */
  register int i, j, t;
  struct keyfield *key = keylist;
  saved.text = NULL;

  /* Read initial lines from each input file. */
  for (i = 0; i < nfps; ++i)
    {
      size_t mergealloc = sort_buffer_size (&fps[i], 1, &files[i], 1,
					    sizeof (struct line),
					    sort_size / nfps);
      initbuf (&buffer[i], sizeof (struct line), mergealloc);
      /* If a file is empty, eliminate it from future consideration. */
      while (i < nfps && !fillbuf (&buffer[i], fps[i], files[i]))
	{
	  xfclose (fps[i]);
	  zaptemp (files[i]);
	  --nfps;
	  for (j = i; j < nfps; ++j)
	    {
	      fps[j] = fps[j + 1];
	      files[j] = files[j + 1];
	    }
	}
      if (i == nfps)
	free (buffer[i].buf);
      else
	{
	  struct line const *linelim = buffer_linelim (&buffer[i]);
	  cur[i] = linelim - 1;
	  base[i] = linelim - buffer[i].nlines;
	}
    }

  /* Set up the ord table according to comparisons among input lines.
     Since this only reorders two items if one is strictly greater than
     the other, it is stable. */
  for (i = 0; i < nfps; ++i)
    ord[i] = i;
  for (i = 1; i < nfps; ++i)
    if (0 < compare (cur[ord[i - 1]], cur[ord[i]]))
      t = ord[i - 1], ord[i - 1] = ord[i], ord[i] = t, i = 0;

  /* Repeatedly output the smallest line until no input remains. */
  while (nfps)
    {
      struct line const *smallest = cur[ord[0]];

      /* If uniquified output is turned on, output only the first of
	 an identical series of lines. */
      if (unique)
	{
	  if (savedline && compare (savedline, smallest))
	    {
	      savedline = 0;
	      write_bytes (saved.text, saved.length, ofp, output_file);
	    }
	  if (!savedline)
	    {
	      savedline = &saved;
	      if (savealloc < smallest->length)
		{
		  do
		    if (! savealloc)
		      {
			savealloc = smallest->length;
			break;
		      }
		  while ((savealloc *= 2) < smallest->length);

		  saved.text = xrealloc (saved.text, savealloc);
		}
	      saved.length = smallest->length;
	      memcpy (saved.text, smallest->text, saved.length);
	      if (key)
		{
		  saved.keybeg =
		    saved.text + (smallest->keybeg - smallest->text);
		  saved.keylim =
		    saved.text + (smallest->keylim - smallest->text);
		}
	    }
	}
      else
	write_bytes (smallest->text, smallest->length, ofp, output_file);

      /* Check if we need to read more lines into core. */
      if (base[ord[0]] < smallest)
	cur[ord[0]] = smallest - 1;
      else
	{
	  if (fillbuf (&buffer[ord[0]], fps[ord[0]], files[ord[0]]))
	    {
	      struct line const *linelim = buffer_linelim (&buffer[ord[0]]);
	      cur[ord[0]] = linelim - 1;
	      base[ord[0]] = linelim - buffer[ord[0]].nlines;
	    }
	  else
	    {
	      /* We reached EOF on fps[ord[0]]. */
	      for (i = 1; i < nfps; ++i)
		if (ord[i] > ord[0])
		  --ord[i];
	      --nfps;
	      xfclose (fps[ord[0]]);
	      zaptemp (files[ord[0]]);
	      free (buffer[ord[0]].buf);
	      for (i = ord[0]; i < nfps; ++i)
		{
		  fps[i] = fps[i + 1];
		  files[i] = files[i + 1];
		  buffer[i] = buffer[i + 1];
		  cur[i] = cur[i + 1];
		  base[i] = base[i + 1];
		}
	      for (i = 0; i < nfps; ++i)
		ord[i] = ord[i + 1];
	      continue;
	    }
	}

      /* The new line just read in may be larger than other lines
	 already in core; push it back in the queue until we encounter
	 a line larger than it. */
      for (i = 1; i < nfps; ++i)
	{
	  t = compare (cur[ord[0]], cur[ord[i]]);
	  if (!t)
	    t = ord[0] - ord[i];
	  if (t < 0)
	    break;
	}
      t = ord[0];
      for (j = 1; j < i; ++j)
	ord[j - 1] = ord[j];
      ord[i - 1] = t;
    }

  if (unique && savedline)
    {
      write_bytes (saved.text, saved.length, ofp, output_file);
      free (saved.text);
    }
}

/* Sort the array LINES with NLINES members, using TEMP for temporary space.
   The input and output arrays are in reverse order, and LINES and
   TEMP point just past the end of their respective arrays.  */

static void
sortlines (struct line *lines, size_t nlines, struct line *temp)
{
  register struct line *lo, *hi, *t;
  register size_t nlo, nhi;

  if (nlines == 2)
    {
      if (0 < compare (&lines[-1], &lines[-2]))
	{
	  struct line tmp = lines[-1];
	  lines[-1] = lines[-2];
	  lines[-2] = tmp;
	}
      return;
    }

  nlo = nlines / 2;
  lo = lines;
  nhi = nlines - nlo;
  hi = lines - nlo;

  if (nlo > 1)
    sortlines (lo, nlo, temp);

  if (nhi > 1)
    sortlines (hi, nhi, temp);

  t = temp;

  while (nlo && nhi)
    if (compare (lo - 1, hi - 1) <= 0)
      *--t = *--lo, --nlo;
    else
      *--t = *--hi, --nhi;
  while (nlo--)
    *--t = *--lo;

  for (lo = lines, nlo = nlines - nhi, t = temp; nlo; --nlo)
    *--lo = *--t;
}

/* Check that each of the NFILES FILES is ordered.
   Return a count of disordered files. */

static int
check (char **files, int nfiles)
{
  int i, disorders = 0;
  FILE *fp;

  for (i = 0; i < nfiles; ++i)
    {
      fp = xfopen (files[i], "r");
      disorders += checkfp (fp, files[i]);
    }
  return disorders;
}

/* Merge NFILES FILES onto OFP. */

static void
merge (char **files, int nfiles, FILE *ofp, const char *output_file)
{
  int i, j, t;
  char *temp;
  FILE *fps[NMERGE], *tfp;

  while (nfiles > NMERGE)
    {
      t = 0;
      for (i = 0; i < nfiles / NMERGE; ++i)
	{
	  for (j = 0; j < NMERGE; ++j)
	    fps[j] = xfopen (files[i * NMERGE + j], "r");
	  temp = create_temp_file (&tfp);
	  mergefps (fps, &files[i * NMERGE], NMERGE, tfp, temp);
	  xfclose (tfp);
	  files[t++] = temp;
	}
      for (j = 0; j < nfiles % NMERGE; ++j)
	fps[j] = xfopen (files[i * NMERGE + j], "r");
      temp = create_temp_file (&tfp);
      mergefps (fps, &files[i * NMERGE], nfiles % NMERGE, tfp, temp);
      xfclose (tfp);
      files[t++] = temp;
      nfiles = t;
    }

  for (i = 0; i < nfiles; ++i)
    fps[i] = xfopen (files[i], "r");
  mergefps (fps, files, nfiles, ofp, output_file);
}

/* Sort NFILES FILES onto OFP. */

static void
sort (char **files, int nfiles, FILE *ofp, const char *output_file)
{
  struct buffer buf;
  FILE *tfp;
  struct tempnode *node;
  int n_temp_files = 0;
  char **tempfiles;

  buf.alloc = 0;

  while (nfiles)
    {
      char const *temp_output;
      char const *file = *files;
      FILE *fp = xfopen (file, "r");

      if (! buf.alloc)
	initbuf (&buf, 2 * sizeof (struct line),
		 sort_buffer_size (&fp, 1, files, nfiles,
				   2 * sizeof (struct line), sort_size));
      buf.eof = 0;
      files++;
      nfiles--;

      while (fillbuf (&buf, fp, file))
	{
	  struct line *line;
	  struct line *linebase;

	  if (buf.eof && nfiles
	      && (2 * sizeof (struct line) + 1
		  < (buf.alloc - buf.used
		     - 2 * sizeof (struct line) * buf.nlines)))
	    {
	      /* End of file, but there is more input and buffer room.
		 Concatenate the next input file; this is faster in
		 the usual case.  */
	      buf.left = buf.used;
	      break;
	    }

	  line = buffer_linelim (&buf);
	  linebase = line - buf.nlines;
	  sortlines (line, buf.nlines, linebase);
	  if (buf.eof && !nfiles && !n_temp_files && !buf.left)
	    {
	      tfp = ofp;
	      temp_output = output_file;
	    }
	  else
	    {
	      ++n_temp_files;
	      temp_output = create_temp_file (&tfp);
	    }

	  do
	    {
	      line--;
	      write_bytes (line->text, line->length, tfp, temp_output);
	      if (unique)
		while (linebase < line && compare (line, line - 1) == 0)
		  line--;
	    }
	  while (linebase < line);

	  if (tfp != ofp)
	    xfclose (tfp);
	}
      xfclose (fp);
    }

  free (buf.buf);

  if (n_temp_files)
    {
      int i = n_temp_files;
      tempfiles = (char **) xmalloc (n_temp_files * sizeof (char *));
      for (node = temphead; i > 0; node = node->next)
	tempfiles[--i] = node->name;
      merge (tempfiles, n_temp_files, ofp, output_file);
      free ((char *) tempfiles);
    }
}

/* Insert key KEY at the end of the key list.  */

static void
insertkey (struct keyfield *key)
{
  struct keyfield **p;

  for (p = &keylist; *p; p = &(*p)->next)
    continue;
  *p = key;
  key->next = NULL;
}

static void
badfieldspec (const char *s)
{
  error (SORT_FAILURE, 0, _("invalid field specification `%s'"), s);
}

/* Parse the leading integer in STRING and store the resulting value
   (which must fit into size_t) into *VAL.  Return the address of the
   suffix after the integer.  */
static char const *
parse_field_count (char const *string, size_t *val)
{
  /* '@' can't possibly be a valid suffix; return &invalid_suffix so that
     the caller will eventually invoke badfieldspec.  */
  static char const invalid_suffix = '@';
  char *suffix;
  uintmax_t n;

  switch (xstrtoumax (string, &suffix, 10, &n, ""))
    {
    case LONGINT_OK:
    case LONGINT_INVALID_SUFFIX_CHAR:
      *val = n;
      if (*val == n)
	break;
      /* Fall through.  */
    case LONGINT_OVERFLOW:
      error (0, 0, _("count `%.*s' too large"),
	     (int) (suffix - string), string);
      return &invalid_suffix;

    case LONGINT_INVALID:
      error (0, 0, _("invalid count at start of `%s'"), string);
      return &invalid_suffix;
    }

  return suffix;
}

/* Handle interrupts and hangups. */

static void
sighandler (int sig)
{
#ifndef SA_NOCLDSTOP
  signal (sig, SIG_IGN);
#endif

  cleanup ();

#ifdef SA_NOCLDSTOP
  {
    struct sigaction sigact;

    sigact.sa_handler = SIG_DFL;
    sigemptyset (&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction (sig, &sigact, NULL);
  }
#else
  signal (sig, SIG_DFL);
#endif

  kill (process_id, sig);
}

/* Set the ordering options for KEY specified in S.
   Return the address of the first character in S that
   is not a valid ordering option.
   BLANKTYPE is the kind of blanks that 'b' should skip. */

static char *
set_ordering (register const char *s, struct keyfield *key,
	      enum blanktype blanktype)
{
  while (*s)
    {
      switch (*s)
	{
	case 'b':
	  if (blanktype == bl_start || blanktype == bl_both)
	    key->skipsblanks = 1;
	  if (blanktype == bl_end || blanktype == bl_both)
	    key->skipeblanks = 1;
	  break;
	case 'd':
	  key->ignore = nondictionary;
	  break;
	case 'f':
	  key->translate = fold_toupper;
	  break;
	case 'g':
	  key->general_numeric = 1;
	  break;
	case 'i':
	  key->ignore = nonprinting;
	  break;
	case 'M':
	  key->month = 1;
	  break;
	case 'n':
	  key->numeric = 1;
	  break;
	case 'r':
	  key->reverse = 1;
	  break;
	default:
	  return (char *) s;
	}
      ++s;
    }
  return (char *) s;
}

static void
key_init (struct keyfield *key)
{
  memset (key, 0, sizeof (*key));
  key->eword = -1;
}

int
main (int argc, char **argv)
{
  struct keyfield *key = NULL, gkey;
  char const *s;
  int i;
  int checkonly = 0, mergeonly = 0, nfiles = 0;
  char *minus = "-", **files, *tmp;
  char const *outfile = minus;
  FILE *ofp;
  static int const sigs[] = { SIGHUP, SIGINT, SIGPIPE, SIGTERM };
  int nsigs = sizeof sigs / sizeof *sigs;
#ifdef SA_NOCLDSTOP
  struct sigaction oldact, newact;
#endif

  program_name = argv[0];
  process_id = getpid ();
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  close_stdout_set_status (SORT_FAILURE);
  atexit (close_stdout);

#ifdef ENABLE_NLS

  hard_LC_COLLATE = hard_locale (LC_COLLATE);
# if HAVE_NL_LANGINFO
  hard_LC_TIME = hard_locale (LC_TIME);
# endif

  /* Let's get locale's representation of the decimal point */
  {
    struct lconv *lconvp = localeconv ();

    /* If the locale doesn't define a decimal point, or if the decimal
       point is multibyte, use the C decimal point.  We don't support
       multibyte decimal points yet.  */
    decimal_point = *lconvp->decimal_point;
    if (! decimal_point || lconvp->decimal_point[1])
      decimal_point = C_DECIMAL_POINT;

    /* We don't support multibyte thousands separators yet.  */
    th_sep = *lconvp->thousands_sep;
    if (! th_sep || lconvp->thousands_sep[1])
      th_sep = CHAR_MAX + 1;
  }

#endif /* NLS */

  parse_long_options (argc, argv, PROGRAM_NAME, PACKAGE, VERSION,
		      AUTHORS, usage);

  have_read_stdin = 0;
  inittables ();

  /* Change the way xmalloc and xrealloc fail.  */
  xalloc_exit_failure = SORT_FAILURE;
  xalloc_fail_func = cleanup;

#ifdef SA_NOCLDSTOP
  sigemptyset (&caught_signals);
  for (i = 0; i < nsigs; i++)
    sigaddset (&caught_signals, sigs[i]);
  newact.sa_handler = sighandler;
  newact.sa_mask = caught_signals;
  newact.sa_flags = 0;
#endif

  for (i = 0; i < nsigs; i++)
    {
      int sig = sigs[i];
#ifdef SA_NOCLDSTOP
      sigaction (sig, NULL, &oldact);
      if (oldact.sa_handler != SIG_IGN)
	sigaction (sig, &newact, NULL);
#else
      if (signal (sig, SIG_IGN) != SIG_IGN)
	signal (sig, sighandler);
#endif
    }

  gkey.sword = gkey.eword = -1;
  gkey.ignore = NULL;
  gkey.translate = NULL;
  gkey.numeric = gkey.general_numeric = gkey.month = gkey.reverse = 0;
  gkey.skipsblanks = gkey.skipeblanks = 0;

  files = (char **) xmalloc (sizeof (char *) * argc);

  for (i = 1; i < argc; ++i)
    {
      if (argv[i][0] == '+')
	{
	  if (key)
	    insertkey (key);
	  key = (struct keyfield *) xmalloc (sizeof (struct keyfield));
	  key_init (key);
	  s = argv[i] + 1;
	  if (! (ISDIGIT (*s) || (*s == '.' && ISDIGIT (s[1]))))
	    badfieldspec (argv[i]);
	  s = parse_field_count (s, &key->sword);
	  if (*s == '.')
	    s = parse_field_count (s + 1, &key->schar);
	  if (! (key->sword | key->schar))
	    key->sword = -1;
	  s = set_ordering (s, key, bl_start);
	  if (*s)
	    badfieldspec (argv[i]);
	}
      else if (argv[i][0] == '-' && argv[i][1])
	{
	  s = argv[i] + 1;
	  if (ISDIGIT (*s) || (*s == '.' && ISDIGIT (s[1])))
	    {
	      if (!key)
		{
		  /* Provoke with `sort -9'.  */
		  error (0, 0, _("when using the old-style +POS and -POS \
key specifiers,\nthe +POS specifier must come first"));
		  usage (SORT_FAILURE);
		}
	      s = parse_field_count (s, &key->eword);
	      if (*s == '.')
		s = parse_field_count (s + 1, &key->echar);
	      s = set_ordering (s, key, bl_end);
	      if (*s)
		badfieldspec (argv[i]);
	      insertkey (key);
	      key = NULL;
	    }
	  else
	    while (*s)
	      {
		s = set_ordering (s, &gkey, bl_both);
		switch (*s)
		  {
		  case '\0':
		    break;
		  case 'c':
		    checkonly = 1;
		    break;
		  case 'k':
		    if (s[1])
		      ++s;
		    else
		      {
			if (i == argc - 1)
			  error (SORT_FAILURE, 0,
				 _("option `-k' requires an argument"));
			else
			  s = argv[++i];
		      }
		    if (key)
		      insertkey (key);
		    key = (struct keyfield *)
		      xmalloc (sizeof (struct keyfield));
		    key_init (key);
		    /* Get POS1. */
		    if (!ISDIGIT (*s))
		      badfieldspec (argv[i]);
		    s = parse_field_count (s, &key->sword);
		    if (! key->sword--)
		      {
			/* Provoke with `sort -k0' */
			error (0, 0, _("the starting field number argument \
to the `-k' option must be positive"));
			badfieldspec (argv[i]);
		      }
		    if (*s == '.')
		      {
			if (!ISDIGIT (s[1]))
			  {
			    /* Provoke with `sort -k1.' */
			    error (0, 0, _("starting field spec has `.' but \
lacks following character offset"));
			    badfieldspec (argv[i]);
			  }
			s = parse_field_count (s + 1, &key->schar);
			if (! key->schar--)
			  {
			    /* Provoke with `sort -k1.0' */
			    error (0, 0, _("starting field character offset \
argument to the `-k' option must be positive"));
			    badfieldspec (argv[i]);
			  }
		      }
		    if (! (key->sword | key->schar))
		      key->sword = -1;
		    s = set_ordering (s, key, bl_start);
		    if (*s == 0)
		      {
			key->eword = -1;
			key->echar = 0;
		      }
		    else if (*s != ',')
		      badfieldspec (argv[i]);
		    else if (*s == ',')
		      {
			/* Skip over comma.  */
			++s;
			if (*s == 0)
			  {
			    /* Provoke with `sort -k1,' */
			    error (0, 0, _("field specification has `,' but \
lacks following field spec"));
			    badfieldspec (argv[i]);
			  }
			/* Get POS2. */
			s = parse_field_count (s, &key->eword);
			if (! key->eword--)
			  {
			    /* Provoke with `sort -k1,0' */
			    error (0, 0, _("ending field number argument \
to the `-k' option must be positive"));
			    badfieldspec (argv[i]);
			  }
			if (*s == '.')
			  {
			    if (!ISDIGIT (s[1]))
			      {
				/* Provoke with `sort -k1,1.' */
				error (0, 0, _("ending field spec has `.' \
but lacks following character offset"));
				badfieldspec (argv[i]);
			      }
			    s = parse_field_count (s + 1, &key->echar);
			  }
			else
			  {
			    /* `-k 2,3' is equivalent to `+1 -3'.  */
			    key->eword++;
			  }
			s = set_ordering (s, key, bl_end);
			if (*s)
			  badfieldspec (argv[i]);
		      }
		    insertkey (key);
		    key = NULL;
		    goto outer;
		  case 'm':
		    mergeonly = 1;
		    break;
		  case 'o':
		    if (s[1])
		      outfile = s + 1;
		    else
		      {
			if (i == argc - 1)
			  error (SORT_FAILURE, 0,
				 _("option `-o' requires an argument"));
			else
			  outfile = argv[++i];
		      }
		    close_stdout_set_file_name (outfile);
		    goto outer;
		  case 's':
		    stable = 1;
		    break;
		  case 'S':
		    if (s[1])
		      specify_sort_size (++s);
		    else if (i < argc - 1)
		      specify_sort_size (argv[++i]);
		    else
		      error (SORT_FAILURE, 0,
			     _("option `-S' requires an argument"));
		    goto outer;
		  case 't':
		    if (s[1])
		      tab = *++s;
		    else if (i < argc - 1)
		      {
			tab = *argv[++i];
			goto outer;
		      }
		    else
		      error (SORT_FAILURE, 0,
			     _("option `-t' requires an argument"));
		    break;
		  case 'T':
		    if (s[1])
		      add_temp_dir (++s);
		    else
		      {
			if (i < argc - 1)
			  add_temp_dir (argv[++i]);
			else
			  error (SORT_FAILURE, 0,
				 _("option `-T' requires an argument"));
		      }
		    goto outer;
		    /* break; */
		  case 'u':
		    unique = 1;
		    break;
		  case 'z':
		    eolchar = 0;
		    break;
		  case 'y':
		    /* Accept and ignore e.g. -y0 for compatibility with
		       Solaris 2.  */
		    goto outer;
		  default:
		    fprintf (stderr, _("%s: unrecognized option `-%c'\n"),
			     argv[0], *s);
		    usage (SORT_FAILURE);
		  }
		if (*s)
		  ++s;
	      }
	}
      else			/* Not an option. */
	{
	  files[nfiles++] = argv[i];
	}
    outer:;
    }

  if (key)
    insertkey (key);

  /* Inheritance of global options to individual keys. */
  for (key = keylist; key; key = key->next)
    if (!key->ignore && !key->translate && !key->skipsblanks && !key->reverse
	&& !key->skipeblanks && !key->month && !key->numeric
	&& !key->general_numeric)
      {
	key->ignore = gkey.ignore;
	key->translate = gkey.translate;
	key->skipsblanks = gkey.skipsblanks;
	key->skipeblanks = gkey.skipeblanks;
	key->month = gkey.month;
	key->numeric = gkey.numeric;
	key->general_numeric = gkey.general_numeric;
	key->reverse = gkey.reverse;
      }

  if (!keylist && (gkey.ignore || gkey.translate || gkey.skipsblanks
		   || gkey.skipeblanks || gkey.month || gkey.numeric
		   || gkey.general_numeric))
    insertkey (&gkey);
  reverse = gkey.reverse;

  if (temp_dir_count == 0)
    {
      char const *tmp_dir = getenv ("TMPDIR");
      add_temp_dir (tmp_dir ? tmp_dir : DEFAULT_TMPDIR);
    }

  if (nfiles == 0)
    {
      nfiles = 1;
      files = &minus;
    }

  if (sort_size == 0)
    sort_size = default_sort_size ();

  if (checkonly)
    {
      if (nfiles > 1)
	error (SORT_FAILURE, 0,
	       _("too many arguments;  with -c, there may be at most\
 one file argument"));

      /* POSIX requires that sort return 1 IFF invoked with -c and the
	 input is not properly sorted.  */
      exit (check (files, nfiles) == 0 ? EXIT_SUCCESS : SORT_OUT_OF_ORDER);
    }

  if (!STREQ (outfile, "-"))
    {
      struct stat outstat;
      if (stat (outfile, &outstat) == 0)
	{
	  /* FIXME: warn about this */
	  /* The following code prevents a race condition when
	     people use the brain dead shell programming idiom:
		  cat file | sort -o file
	     This feature is provided for historical compatibility,
	     but we strongly discourage ever relying on this in
	     new shell programs. */

	  /* Temporarily copy each input file that might be another name
	     for the output file.  When in doubt (e.g. a pipe), copy.  */
	  for (i = 0; i < nfiles; ++i)
	    {
	      char buf[8192];
	      FILE *in_fp;
	      FILE *out_fp;
	      int cc;

	      if (S_ISREG (outstat.st_mode) && !STREQ (outfile, files[i]))
		{
		  struct stat instat;
		  if ((STREQ (files[i], "-")
		       ? fstat (STDIN_FILENO, &instat)
		       : stat (files[i], &instat)) != 0)
		    {
		      error (0, errno, "%s", files[i]);
		      cleanup ();
		      exit (SORT_FAILURE);
		    }
		  if (S_ISREG (instat.st_mode) && !SAME_INODE (instat, outstat))
		    {
		      /* We know the files are distinct.  */
		      continue;
		    }
		}

	      in_fp = xfopen (files[i], "r");
	      tmp = create_temp_file (&out_fp);
	      /* FIXME: maybe use copy.c(copy) here. */
	      while ((cc = fread (buf, 1, sizeof buf, in_fp)) > 0)
		write_bytes (buf, cc, out_fp, tmp);
	      if (ferror (in_fp))
		{
		  error (0, errno, "%s", files[i]);
		  cleanup ();
		  exit (SORT_FAILURE);
		}
	      xfclose (out_fp);
	      xfclose (in_fp);
	      files[i] = tmp;
	    }
	  ofp = xfopen (outfile, "w");
	}
      else
	{
	  /* A non-`-' outfile was specified, but the file doesn't yet exist.
	     Before opening it for writing (thus creating it), make sure all
	     of the input files exist.  Otherwise, creating the output file
	     could create an otherwise missing input file, making sort succeed
	     when it should fail.  */
	  for (i = 0; i < nfiles; ++i)
	    {
	      struct stat sb;
	      if (STREQ (files[i], "-"))
		continue;
	      if (stat (files[i], &sb))
		{
		  error (0, errno, "%s", files[i]);
		  cleanup ();
		  exit (SORT_FAILURE);
		}
	    }

	  ofp = xfopen (outfile, "w");
	}
    }
  else
    {
      ofp = stdout;
    }

  if (mergeonly)
    merge (files, nfiles, ofp, outfile);
  else
    sort (files, nfiles, ofp, outfile);
  cleanup ();

  /* If we wait for the implicit flush on exit, and the parent process
     has closed stdout (e.g., exec >&- in a shell), then the output file
     winds up empty.  I don't understand why.  This is under SunOS,
     Solaris, Ultrix, and Irix.  This premature fflush makes the output
     reappear. --karl@cs.umb.edu  */
  if (fflush (ofp) < 0)
    error (SORT_FAILURE, errno, _("%s: write error"), outfile);

  if (have_read_stdin && fclose (stdin) == EOF)
    error (SORT_FAILURE, errno, "%s", outfile);

  exit (EXIT_SUCCESS);
}
