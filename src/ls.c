/* `dir', `vdir' and `ls' directory listing programs for GNU.
   Copyright (C) 1985, 1988, 1990, 1991, 1995 Free Software Foundation, Inc.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* If the macro MULTI_COL is defined,
   the multi-column format is the default regardless
   of the type of output device.
   This is for the `dir' program.

   If the macro LONG_FORMAT is defined,
   the long format is the default regardless of the
   type of output device.
   This is for the `vdir' program.

   If neither is defined,
   the output format depends on whether the output
   device is a terminal.
   This is for the `ls' program. */

/* Written by Richard Stallman and David MacKenzie. */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <sys/types.h>

#if HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <getopt.h>

#if HAVE_LIMITS_H
/* limits.h must come before system.h because limits.h on some systems
   undefs PATH_MAX, whereas system.h includes pathmax.h which sets
   PATH_MAX.  */
#include <limits.h>
#endif

#include "system.h"
#include <fnmatch.h>

#include "obstack.h"
#include "ls.h"
#include "error.h"
#include "argmatch.h"
#include "xstrtol.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

/* Return an int indicating the result of comparing two longs. */
#if (INT_MAX <= 65535)
#define longdiff(a, b) ((a) < (b) ? -1 : (a) > (b) ? 1 : 0)
#else
#define longdiff(a, b) ((a) - (b))
#endif

/* The maximum number of digits required to print an inode number
   in an unsigned format.  */
#ifndef INODE_DIGITS
#define INODE_DIGITS 7
#endif

enum filetype
  {
    symbolic_link,
    directory,
    arg_directory,		/* Directory given as command line arg. */
    normal			/* All others. */
  };

struct fileinfo
  {
    /* The file name. */
    char *name;

    struct stat stat;

    /* For symbolic link, name of the file linked to, otherwise zero. */
    char *linkname;

    /* For symbolic link and long listing, st_mode of file linked to, otherwise
       zero. */
    unsigned int linkmode;

    enum filetype filetype;
  };

#ifndef STDC_HEADERS
char *ctime ();
time_t time ();
void free ();
#endif

void mode_string ();

char *stpcpy ();
char *xstrdup ();
char *getgroup ();
char *getuser ();
char *xmalloc ();
char *xrealloc ();
void invalid_arg ();

static char *make_link_path __P ((char *path, char *linkname));
static int compare_atime __P ((struct fileinfo *file1,
			       struct fileinfo *file2));
static int rev_cmp_atime __P ((struct fileinfo *file2,
			       struct fileinfo *file1));
static int compare_ctime __P ((struct fileinfo *file1,
			        struct fileinfo *file2));
static int rev_cmp_ctime __P ((struct fileinfo *file2,
			        struct fileinfo *file1));
static int compare_mtime __P ((struct fileinfo *file1,
			        struct fileinfo *file2));
static int rev_cmp_mtime __P ((struct fileinfo *file2,
			        struct fileinfo *file1));
static int compare_size __P ((struct fileinfo *file1,
			        struct fileinfo *file2));
static int rev_cmp_size __P ((struct fileinfo *file2,
			        struct fileinfo *file1));
static int compare_name __P ((struct fileinfo *file1,
			        struct fileinfo *file2));
static int rev_cmp_name __P ((struct fileinfo *file2,
			        struct fileinfo *file1));
static int compare_extension __P ((struct fileinfo *file1,
			        struct fileinfo *file2));
static int rev_cmp_extension __P ((struct fileinfo *file2,
			        struct fileinfo *file1));
static int decode_switches __P ((int argc, char **argv));
static int file_interesting __P ((register struct dirent *next));
static int gobble_file __P ((const char *name, int explicit_arg,
			     const char *dirname));
static int is_not_dot_or_dotdot __P ((char *name));
static int length_of_file_name_and_frills __P ((struct fileinfo *f));
static void add_ignore_pattern __P ((char *pattern));
static void attach __P ((char *dest, const char *dirname, const char *name));
static void clear_files __P ((void));
static void extract_dirs_from_files __P ((const char *dirname, int recursive));
static void get_link_name __P ((char *filename, struct fileinfo *f));
static void indent __P ((int from, int to));
static void print_current_files __P ((void));
static void print_dir __P ((const char *name, const char *realname));
static void print_file_name_and_frills __P ((struct fileinfo *f));
static void print_horizontal __P ((void));
static void print_long_format __P ((struct fileinfo *f));
static void print_many_per_line __P ((void));
static void print_name_with_quoting __P ((register char *p));
static void print_type_indicator __P ((unsigned int mode));
static void print_with_commas __P ((void));
static void queue_directory __P ((char *name, char *realname));
static void sort_files __P ((void));
static void usage __P ((int status));

/* The name the program was run with, stripped of any leading path. */
char *program_name;

/* The table of files in the current directory:

   `files' points to a vector of `struct fileinfo', one per file.
   `nfiles' is the number of elements space has been allocated for.
   `files_index' is the number actually in use.  */

/* Address of block containing the files that are described.  */

static struct fileinfo *files;

/* Length of block that `files' points to, measured in files.  */

static int nfiles;

/* Index of first unused in `files'.  */

static int files_index;

/* Record of one pending directory waiting to be listed.  */

struct pending
  {
    char *name;
    /* If the directory is actually the file pointed to by a symbolic link we
       were told to list, `realname' will contain the name of the symbolic
       link, otherwise zero. */
    char *realname;
    struct pending *next;
  };

static struct pending *pending_dirs;

/* Current time (seconds since 1970).  When we are printing a file's time,
   include the year if it is more than 6 months before this time.  */

static time_t current_time;

/* The number of digits to use for block sizes.
   4, or more if needed for bigger numbers.  */

static int block_size_size;

/* Option flags */

/* long_format for lots of info, one per line.
   one_per_line for just names, one per line.
   many_per_line for just names, many per line, sorted vertically.
   horizontal for just names, many per line, sorted horizontally.
   with_commas for just names, many per line, separated by commas.

   -l, -1, -C, -x and -m control this parameter.  */

enum format
  {
    long_format,		/* -l */
    one_per_line,		/* -1 */
    many_per_line,		/* -C */
    horizontal,			/* -x */
    with_commas			/* -m */
  };

static enum format format;

/* Type of time to print or sort by.  Controlled by -c and -u.  */

enum time_type
  {
    time_mtime,			/* default */
    time_ctime,			/* -c */
    time_atime			/* -u */
  };

static enum time_type time_type;

/* print the full time, otherwise the standard unix heuristics. */

int full_time;

/* The file characteristic to sort by.  Controlled by -t, -S, -U, -X. */

enum sort_type
  {
    sort_none,			/* -U */
    sort_name,			/* default */
    sort_extension,		/* -X */
    sort_time,			/* -t */
    sort_size			/* -S */
  };

static enum sort_type sort_type;

/* Direction of sort.
   0 means highest first if numeric,
   lowest first if alphabetic;
   these are the defaults.
   1 means the opposite order in each case.  -r  */

static int sort_reverse;

/* Nonzero means to NOT display group information.  -G  */

int inhibit_group;

/* Nonzero means print the user and group id's as numbers rather
   than as names.  -n  */

static int numeric_users;

/* Nonzero means mention the size in 512 byte blocks of each file.  -s  */

static int print_block_size;

/* Nonzero means show file sizes in kilobytes instead of blocks
   (the size of which is system-dependent).  -k */

static int kilobyte_blocks;

/* Precede each line of long output (per file) with a string like `m,n:'
   where M is the number of characters after the `:' and before the
   filename and N is the length of the filename.  Using this format,
   Emacs' dired mode starts up twice as fast, and can handle all
   strange characters in file names.  */
static int dired;

/* none means don't mention the type of files.
   all means mention the types of all files.
   not_programs means do so except for executables.

   Controlled by -F and -p.  */

enum indicator_style
  {
    none,                      /* default */
    all,                       /* -F */
    not_programs               /* -p */
  };

static enum indicator_style indicator_style;

/* Nonzero means mention the inode number of each file.  -i  */

static int print_inode;

/* Nonzero means when a symbolic link is found, display info on
   the file linked to.  -L  */

static int trace_links;

/* Nonzero means when a directory is found, display info on its
   contents.  -R  */

static int trace_dirs;

/* Nonzero means when an argument is a directory name, display info
   on it itself.  -d  */

static int immediate_dirs;

/* Nonzero means don't omit files whose names start with `.'.  -A */

static int all_files;

/* Nonzero means don't omit files `.' and `..'
   This flag implies `all_files'.  -a  */

static int really_all_files;

/* A linked list of shell-style globbing patterns.  If a non-argument
   file name matches any of these patterns, it is omitted.
   Controlled by -I.  Multiple -I options accumulate.
   The -B option adds `*~' and `.*~' to this list.  */

struct ignore_pattern
  {
    char *pattern;
    struct ignore_pattern *next;
  };

static struct ignore_pattern *ignore_patterns;

/* Nonzero means quote nongraphic chars in file names.  -b  */

static int quote_funny_chars;

/* Nonzero means output nongraphic chars in file names as `?'.  -q  */

static int qmark_funny_chars;

/* Nonzero means output each file name using C syntax for a string.
   Always accompanied by `quote_funny_chars'.
   This mode, together with -x or -C or -m,
   and without such frills as -F or -s,
   is guaranteed to make it possible for a program receiving
   the output to tell exactly what file names are present.  -Q  */

static int quote_as_string;

/* The number of chars per hardware tab stop.  -T */
static int tabsize;

/* Nonzero means we are listing the working directory because no
   non-option arguments were given. */

static int dir_defaulted;

/* Nonzero means print each directory name before listing it. */

static int print_dir_name;

/* The line length to use for breaking lines in many-per-line format.
   Can be set with -w.  */

static int line_length;

/* If nonzero, the file listing format requires that stat be called on
   each file. */

static int format_needs_stat;

/* The exit status to use if we don't get any fatal errors. */

static int exit_status;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"all", no_argument, 0, 'a'},
  {"escape", no_argument, 0, 'b'},
  {"directory", no_argument, 0, 'd'},
  {"dired", no_argument, 0, 'D'},
  {"full-time", no_argument, &full_time, 1},
  {"inode", no_argument, 0, 'i'},
  {"kilobytes", no_argument, 0, 'k'},
  {"numeric-uid-gid", no_argument, 0, 'n'},
  {"no-group", no_argument, 0, 'G'},
  {"hide-control-chars", no_argument, 0, 'q'},
  {"reverse", no_argument, 0, 'r'},
  {"size", no_argument, 0, 's'},
  {"width", required_argument, 0, 'w'},
  {"almost-all", no_argument, 0, 'A'},
  {"ignore-backups", no_argument, 0, 'B'},
  {"classify", no_argument, 0, 'F'},
  {"file-type", no_argument, 0, 'F'},
  {"ignore", required_argument, 0, 'I'},
  {"dereference", no_argument, 0, 'L'},
  {"literal", no_argument, 0, 'N'},
  {"quote-name", no_argument, 0, 'Q'},
  {"recursive", no_argument, 0, 'R'},
  {"format", required_argument, 0, 12},
  {"sort", required_argument, 0, 10},
  {"tabsize", required_argument, 0, 'T'},
  {"time", required_argument, 0, 11},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

static char const *const format_args[] =
{
  "verbose", "long", "commas", "horizontal", "across",
  "vertical", "single-column", 0
};

static enum format const formats[] =
{
  long_format, long_format, with_commas, horizontal, horizontal,
  many_per_line, one_per_line
};

static char const *const sort_args[] =
{
  "none", "time", "size", "extension", 0
};

static enum sort_type const sort_types[] =
{
  sort_none, sort_time, sort_size, sort_extension
};

static char const *const time_args[] =
{
  "atime", "access", "use", "ctime", "status", 0
};

/* This zero-based index is used solely with the --dired option.
   When that option is in effect, this counter is incremented for each
   character of output generated by this program so that the beginning
   and ending indices (in that output) of every file name can be recorded
   and later output themselves.  */
static size_t dired_pos;

#define PUTCHAR(c) do {putchar ((c)); ++dired_pos;} while (0)

/* Write S to STREAM and increment DIRED_POS by S_LEN.  */
#define FPUTS(s, stream, s_len) \
    do {fputs ((s), (stream)); dired_pos += s_len;} while (0)

/* Like FPUTS, but for use when S is a literal string.  */
#define FPUTS_LITERAL(s, stream) \
    do {fputs ((s), (stream)); dired_pos += sizeof((s)) - 1;} while (0)

#define DIRED_INDENT()							\
    do									\
      {									\
	/* FIXME: remove the `&& format == long_format' clause.  */	\
	if (dired && format == long_format)				\
	  FPUTS_LITERAL ("  ", stdout);					\
      }									\
    while (0)

/* With --dired, store pairs of beginning and ending indices of filenames.  */
static struct obstack dired_obstack;

/* With --dired, store pairs of beginning and ending indices of any
   directory names that appear as headers (just before `total' line)
   for lists of directory entries.  Such directory names are seen when
   listing hierarchies using -R and when a directory is listed with at
   least one other command line argument.  */
static struct obstack subdired_obstack;

/* Save the current index on the specified obstack, OBS.  */
#define PUSH_CURRENT_DIRED_POS(obs)					\
  do									\
    {									\
      /* FIXME: remove the `&& format == long_format' clause.  */	\
      if (dired && format == long_format)				\
	obstack_grow ((obs), &dired_pos, sizeof (dired_pos));		\
    }									\
  while (0)

static enum time_type const time_types[] =
{
  time_atime, time_atime, time_atime, time_ctime, time_ctime
};


/* Write to standard output the string PREFIX followed by a space-separated
   list of the integers stored in OS all on one line.  */

static void
dired_dump_obstack (const char *prefix, struct obstack *os)
{
  int n_pos;

  n_pos = obstack_object_size (os) / sizeof (size_t);
  if (n_pos > 0)
    {
      int i;
      size_t *pos;

      pos = (size_t *) obstack_finish (os);
      fputs (prefix, stdout);
      for (i = 0; i < n_pos; i++)
	printf (" %d", (int) pos[i]);
      fputs ("\n", stdout);
    }
}

int
main (int argc, char **argv)
{
  register int i;
  register struct pending *thispend;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  exit_status = 0;
  dir_defaulted = 1;
  print_dir_name = 1;
  pending_dirs = 0;
  current_time = time ((time_t *) 0);

  i = decode_switches (argc, argv);

  if (show_version)
    {
      printf ("ls - %s\n", PACKAGE_VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  format_needs_stat = sort_type == sort_time || sort_type == sort_size
    || format == long_format
    || trace_links || trace_dirs || indicator_style != none
    || print_block_size || print_inode;

  if (dired && format == long_format)
    {
      obstack_init (&dired_obstack);
      obstack_init (&subdired_obstack);
    }

  nfiles = 100;
  files = (struct fileinfo *) xmalloc (sizeof (struct fileinfo) * nfiles);
  files_index = 0;

  clear_files ();

  if (i < argc)
    dir_defaulted = 0;
  for (; i < argc; i++)
    gobble_file (argv[i], 1, "");

  if (dir_defaulted)
    {
      if (immediate_dirs)
	gobble_file (".", 1, "");
      else
	queue_directory (".", 0);
    }

  if (files_index)
    {
      sort_files ();
      if (!immediate_dirs)
	extract_dirs_from_files ("", 0);
      /* `files_index' might be zero now.  */
    }
  if (files_index)
    {
      print_current_files ();
      if (pending_dirs)
	PUTCHAR ('\n');
    }
  else if (pending_dirs && pending_dirs->next == 0)
    print_dir_name = 0;

  while (pending_dirs)
    {
      thispend = pending_dirs;
      pending_dirs = pending_dirs->next;
      print_dir (thispend->name, thispend->realname);
      free (thispend->name);
      if (thispend->realname)
	free (thispend->realname);
      free (thispend);
      print_dir_name = 1;
    }

  if (dired && format == long_format)
    {
      /* No need to free these since we're about to exit.  */
      dired_dump_obstack ("//DIRED//", &dired_obstack);
      dired_dump_obstack ("//SUBDIRED//", &subdired_obstack);
    }

  exit (exit_status);
}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */

static int
decode_switches (int argc, char **argv)
{
  register char *p;
  int c;
  int i;
  long int tmp_long;

  qmark_funny_chars = 0;
  quote_funny_chars = 0;

  /* initialize all switches to default settings */

  switch (ls_mode)
    {
    case LS_MULTI_COL:
      /* This is for the `dir' program.  */
      format = many_per_line;
      quote_funny_chars = 1;
      break;

    case LS_LONG_FORMAT:
      /* This is for the `vdir' program.  */
      format = long_format;
      quote_funny_chars = 1;
      break;

    case LS_LS:
      /* This is for the `ls' program.  */
      if (isatty (1))
	{
	  format = many_per_line;
	  qmark_funny_chars = 1;
	}
      else
	{
	  format = one_per_line;
	  qmark_funny_chars = 0;
	}
      break;

    default:
      abort ();
    }

  time_type = time_mtime;
  full_time = 0;
  sort_type = sort_name;
  sort_reverse = 0;
  numeric_users = 0;
  print_block_size = 0;
  kilobyte_blocks = getenv ("POSIXLY_CORRECT") == 0;
  indicator_style = none;
  print_inode = 0;
  trace_links = 0;
  trace_dirs = 0;
  immediate_dirs = 0;
  all_files = 0;
  really_all_files = 0;
  ignore_patterns = 0;
  quote_as_string = 0;

  line_length = 80;
  if ((p = getenv ("COLUMNS")))
    {
      if (xstrtol (p, NULL, 0, &tmp_long, NULL) == LONGINT_OK
	  && 0 < tmp_long && tmp_long <= INT_MAX)
	{
	  line_length = (int) tmp_long;
	}
      else
	{
	  error (0, 0,
	       _("ignoring invalid width in enironment variable COLUMNS: %s"),
		 p);
	}
    }

#ifdef TIOCGWINSZ
  {
    struct winsize ws;

    if (ioctl (1, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0)
      line_length = ws.ws_col;
  }
#endif

  /* TABSIZE is not POSIX-approved.
     Ignore it when POSIXLY_CORRECT is set.  */
  tabsize = 8;
  if (getenv ("POSIXLY_CORRECT") == 0 && (p = getenv ("TABSIZE")))
    {
      if (xstrtol (p, NULL, 0, &tmp_long, NULL) == LONGINT_OK
	  && 0 < tmp_long && tmp_long <= INT_MAX)
	{
	  tabsize = (int) tmp_long;
	}
      else
	{
	  error (0, 0,
	     _("ignoring invalid tab size in enironment variable TABSIZE: %s"),
		 p);
	}
    }

  while ((c = getopt_long (argc, argv,
			   "abcdfgiklmnopqrstuw:xABCDFGI:LNQRST:UX1",
			   long_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 0:
	  break;

	case 'a':
	  all_files = 1;
	  really_all_files = 1;
	  break;

	case 'b':
	  quote_funny_chars = 1;
	  qmark_funny_chars = 0;
	  break;

	case 'c':
	  time_type = time_ctime;
	  sort_type = sort_time;
	  break;

	case 'd':
	  immediate_dirs = 1;
	  break;

	case 'f':
	  /* Same as enabling -a -U and disabling -l -s.  */
	  all_files = 1;
	  really_all_files = 1;
	  sort_type = sort_none;
	  /* disable -l */
	  if (format == long_format)
	    format = (isatty (1) ? many_per_line : one_per_line);
	  print_block_size = 0;	/* disable -s */
	  break;

	case 'g':
	  /* No effect.  For BSD compatibility. */
	  break;

	case 'i':
	  print_inode = 1;
	  break;

	case 'k':
	  kilobyte_blocks = 1;
	  break;

	case 'l':
	  format = long_format;
	  break;

	case 'm':
	  format = with_commas;
	  break;

	case 'n':
	  numeric_users = 1;
	  break;

	case 'o':  /* Just like -l, but don't display group info.  */
	  format = long_format;
	  inhibit_group = 1;
	  break;

	case 'p':
	  indicator_style = not_programs;
	  break;

	case 'q':
	  qmark_funny_chars = 1;
	  quote_funny_chars = 0;
	  break;

	case 'r':
	  sort_reverse = 1;
	  break;

	case 's':
	  print_block_size = 1;
	  break;

	case 't':
	  sort_type = sort_time;
	  break;

	case 'u':
	  time_type = time_atime;
	  break;

	case 'w':
	  line_length = atoi (optarg);
	  if (line_length < 1)
	    error (1, 0, _("invalid line width: %s"), optarg);
	  break;

	case 'x':
	  format = horizontal;
	  break;

	case 'A':
	  all_files = 1;
	  break;

	case 'B':
	  add_ignore_pattern ("*~");
	  add_ignore_pattern (".*~");
	  break;

	case 'C':
	  format = many_per_line;
	  break;

	case 'D':
	  dired = 1;
	  break;

	case 'F':
	  indicator_style = all;
	  break;

	case 'G':		/* inhibit display of group info */
	  inhibit_group = 1;
	  break;

	case 'I':
	  add_ignore_pattern (optarg);
	  break;

	case 'L':
	  trace_links = 1;
	  break;

	case 'N':
	  quote_funny_chars = 0;
	  qmark_funny_chars = 0;
	  break;

	case 'Q':
	  quote_as_string = 1;
	  quote_funny_chars = 1;
	  qmark_funny_chars = 0;
	  break;

	case 'R':
	  trace_dirs = 1;
	  break;

	case 'S':
	  sort_type = sort_size;
	  break;

	case 'T':
	  tabsize = atoi (optarg);
	  if (tabsize < 1)
	    error (1, 0, _("invalid tab size: %s"), optarg);
	  break;

	case 'U':
	  sort_type = sort_none;
	  break;

	case 'X':
	  sort_type = sort_extension;
	  break;

	case '1':
	  format = one_per_line;
	  break;

	case 10:		/* --sort */
	  i = argmatch (optarg, sort_args);
	  if (i < 0)
	    {
	      invalid_arg (_("sort type"), optarg, i);
	      usage (1);
	    }
	  sort_type = sort_types[i];
	  break;

	case 11:		/* --time */
	  i = argmatch (optarg, time_args);
	  if (i < 0)
	    {
	      invalid_arg (_("time type"), optarg, i);
	      usage (1);
	    }
	  time_type = time_types[i];
	  break;

	case 12:		/* --format */
	  i = argmatch (optarg, format_args);
	  if (i < 0)
	    {
	      invalid_arg (_("format type"), optarg, i);
	      usage (1);
	    }
	  format = formats[i];
	  break;

	default:
	  usage (1);
	}
    }

  return optind;
}

/* Request that the directory named `name' have its contents listed later.
   If `realname' is nonzero, it will be used instead of `name' when the
   directory name is printed.  This allows symbolic links to directories
   to be treated as regular directories but still be listed under their
   real names. */

static void
queue_directory (char *name, char *realname)
{
  struct pending *new;

  new = (struct pending *) xmalloc (sizeof (struct pending));
  new->next = pending_dirs;
  pending_dirs = new;
  new->name = xstrdup (name);
  if (realname)
    new->realname = xstrdup (realname);
  else
    new->realname = 0;
}

/* Read directory `name', and list the files in it.
   If `realname' is nonzero, print its name instead of `name';
   this is used for symbolic links to directories. */

static void
print_dir (const char *name, const char *realname)
{
  register DIR *reading;
  register struct dirent *next;
  register int total_blocks = 0;

  errno = 0;
  reading = opendir (name);
  if (!reading)
    {
      error (0, errno, "%s", name);
      exit_status = 1;
      return;
    }

  /* Read the directory entries, and insert the subfiles into the `files'
     table.  */

  clear_files ();

  while ((next = readdir (reading)) != NULL)
    if (file_interesting (next))
      total_blocks += gobble_file (next->d_name, 0, name);

  if (CLOSEDIR (reading))
    {
      error (0, errno, "%s", name);
      exit_status = 1;
      /* Don't return; print whatever we got. */
    }

  /* Sort the directory contents.  */
  sort_files ();

  /* If any member files are subdirectories, perhaps they should have their
     contents listed rather than being mentioned here as files.  */

  if (trace_dirs)
    extract_dirs_from_files (name, 1);

  if (print_dir_name)
    {
      const char *dir;

      DIRED_INDENT ();
      dir = (realname ? realname : name);
      PUSH_CURRENT_DIRED_POS (&subdired_obstack);
      FPUTS (dir, stdout, strlen (dir));
      PUSH_CURRENT_DIRED_POS (&subdired_obstack);
      FPUTS_LITERAL (":\n", stdout);
    }

  if (format == long_format || print_block_size)
    {
      char buf[6 + 20 + 1 + 1];

      DIRED_INDENT ();
      sprintf (buf, "total %u\n", total_blocks);
      FPUTS (buf, stdout, strlen (buf));
    }

  if (files_index)
    print_current_files ();

  if (pending_dirs)
    PUTCHAR ('\n');
}

/* Add `pattern' to the list of patterns for which files that match are
   not listed.  */

static void
add_ignore_pattern (char *pattern)
{
  register struct ignore_pattern *ignore;

  ignore = (struct ignore_pattern *) xmalloc (sizeof (struct ignore_pattern));
  ignore->pattern = pattern;
  /* Add it to the head of the linked list. */
  ignore->next = ignore_patterns;
  ignore_patterns = ignore;
}

/* Return nonzero if the file in `next' should be listed. */

static int
file_interesting (register struct dirent *next)
{
  register struct ignore_pattern *ignore;

  for (ignore = ignore_patterns; ignore; ignore = ignore->next)
    if (fnmatch (ignore->pattern, next->d_name, FNM_PERIOD) == 0)
      return 0;

  if (really_all_files
      || next->d_name[0] != '.'
      || (all_files
	  && next->d_name[1] != '\0'
	  && (next->d_name[1] != '.' || next->d_name[2] != '\0')))
    return 1;

  return 0;
}

/* Enter and remove entries in the table `files'.  */

/* Empty the table of files. */

static void
clear_files (void)
{
  register int i;

  for (i = 0; i < files_index; i++)
    {
      free (files[i].name);
      if (files[i].linkname)
	free (files[i].linkname);
    }

  files_index = 0;
  block_size_size = 4;
}

/* Add a file to the current table of files.
   Verify that the file exists, and print an error message if it does not.
   Return the number of blocks that the file occupies.  */

static int
gobble_file (const char *name, int explicit_arg, const char *dirname)
{
  register int blocks;
  register int val;
  register char *path;

  if (files_index == nfiles)
    {
      nfiles *= 2;
      files = (struct fileinfo *) xrealloc (files, sizeof (*files) * nfiles);
    }

  files[files_index].linkname = 0;
  files[files_index].linkmode = 0;

  if (explicit_arg || format_needs_stat)
    {
      /* `path' is the absolute pathname of this file. */

      if (name[0] == '/' || dirname[0] == 0)
	path = (char *) name;
      else
	{
	  path = (char *) alloca (strlen (name) + strlen (dirname) + 2);
	  attach (path, dirname, name);
	}

      if (trace_links)
	{
	  val = stat (path, &files[files_index].stat);
	  if (val < 0)
	    /* Perhaps a symbolically-linked to file doesn't exist; stat
	       the link instead. */
	    val = lstat (path, &files[files_index].stat);
	}
      else
	val = lstat (path, &files[files_index].stat);
      if (val < 0)
	{
	  error (0, errno, "%s", path);
	  exit_status = 1;
	  return 0;
	}

#ifdef S_ISLNK
      if (S_ISLNK (files[files_index].stat.st_mode)
	  && (explicit_arg || format == long_format))
	{
	  char *linkpath;
	  struct stat linkstats;

	  get_link_name (path, &files[files_index]);
	  linkpath = make_link_path (path, files[files_index].linkname);

	  /* Avoid following symbolic links when possible, ie, when
	     they won't be traced and when no indicator is needed. */
	  if (linkpath
	      && ((explicit_arg && format != long_format)
		  || indicator_style != none)
	      && stat (linkpath, &linkstats) == 0)
	    {
	      /* Symbolic links to directories that are mentioned on the
	         command line are automatically traced if not being
	         listed as files.  */
	      if (explicit_arg && format != long_format
		  && S_ISDIR (linkstats.st_mode))
		{
		  /* Substitute the linked-to directory's name, but
		     save the real name in `linkname' for printing.  */
		  if (!immediate_dirs)
		    {
		      const char *tempname = name;
		      name = linkpath;
		      linkpath = files[files_index].linkname;
		      files[files_index].linkname = (char *) tempname;
		    }
		  files[files_index].stat = linkstats;
		}
	      else
		/* Get the linked-to file's mode for the filetype indicator
		   in long listings.  */
		files[files_index].linkmode = linkstats.st_mode;
	    }
	  if (linkpath)
	    free (linkpath);
	}
#endif

#ifdef S_ISLNK
      if (S_ISLNK (files[files_index].stat.st_mode))
	files[files_index].filetype = symbolic_link;
      else
#endif
      if (S_ISDIR (files[files_index].stat.st_mode))
	{
	  if (explicit_arg && !immediate_dirs)
	    files[files_index].filetype = arg_directory;
	  else
	    files[files_index].filetype = directory;
	}
      else
	files[files_index].filetype = normal;

      blocks = convert_blocks (ST_NBLOCKS (files[files_index].stat),
			       kilobyte_blocks);
      if (blocks >= 10000 && block_size_size < 5)
	block_size_size = 5;
      if (blocks >= 100000 && block_size_size < 6)
	block_size_size = 6;
      if (blocks >= 1000000 && block_size_size < 7)
	block_size_size = 7;
    }
  else
    blocks = 0;

  files[files_index].name = xstrdup (name);
  files_index++;

  return blocks;
}

#ifdef S_ISLNK

/* Put the name of the file that `filename' is a symbolic link to
   into the `linkname' field of `f'. */

static void
get_link_name (char *filename, struct fileinfo *f)
{
  char *linkbuf;
  register int linksize;

  linkbuf = (char *) alloca (PATH_MAX + 2);
  /* Some automounters give incorrect st_size for mount points.
     I can't think of a good workaround for it, though.  */
  linksize = readlink (filename, linkbuf, PATH_MAX + 1);
  if (linksize < 0)
    {
      error (0, errno, "%s", filename);
      exit_status = 1;
    }
  else
    {
      linkbuf[linksize] = '\0';
      f->linkname = xstrdup (linkbuf);
    }
}

/* If `linkname' is a relative path and `path' contains one or more
   leading directories, return `linkname' with those directories
   prepended; otherwise, return a copy of `linkname'.
   If `linkname' is zero, return zero. */

static char *
make_link_path (char *path, char *linkname)
{
  char *linkbuf;
  int bufsiz;

  if (linkname == 0)
    return 0;

  if (*linkname == '/')
    return xstrdup (linkname);

  /* The link is to a relative path.  Prepend any leading path
     in `path' to the link name. */
  linkbuf = strrchr (path, '/');
  if (linkbuf == 0)
    return xstrdup (linkname);

  bufsiz = linkbuf - path + 1;
  linkbuf = xmalloc (bufsiz + strlen (linkname) + 1);
  strncpy (linkbuf, path, bufsiz);
  strcpy (linkbuf + bufsiz, linkname);
  return linkbuf;
}
#endif

/* Remove any entries from `files' that are for directories,
   and queue them to be listed as directories instead.
   `dirname' is the prefix to prepend to each dirname
   to make it correct relative to ls's working dir.
   `recursive' is nonzero if we should not treat `.' and `..' as dirs.
   This is desirable when processing directories recursively.  */

static void
extract_dirs_from_files (const char *dirname, int recursive)
{
  register int i, j;
  register char *path;
  int dirlen;

  dirlen = strlen (dirname) + 2;
  /* Queue the directories last one first, because queueing reverses the
     order.  */
  for (i = files_index - 1; i >= 0; i--)
    if ((files[i].filetype == directory || files[i].filetype == arg_directory)
	&& (!recursive || is_not_dot_or_dotdot (files[i].name)))
      {
	if (files[i].name[0] == '/' || dirname[0] == 0)
	  {
	    queue_directory (files[i].name, files[i].linkname);
	  }
	else
	  {
	    path = (char *) xmalloc (strlen (files[i].name) + dirlen);
	    attach (path, dirname, files[i].name);
	    queue_directory (path, files[i].linkname);
	    free (path);
	  }
	if (files[i].filetype == arg_directory)
	  free (files[i].name);
      }

  /* Now delete the directories from the table, compacting all the remaining
     entries.  */

  for (i = 0, j = 0; i < files_index; i++)
    if (files[i].filetype != arg_directory)
      files[j++] = files[i];
  files_index = j;
}

/* Return nonzero if `name' doesn't end in `.' or `..'
   This is so we don't try to recurse on `././././. ...' */

static int
is_not_dot_or_dotdot (char *name)
{
  char *t;

  t = strrchr (name, '/');
  if (t)
    name = t + 1;

  if (name[0] == '.'
      && (name[1] == '\0'
	  || (name[1] == '.' && name[2] == '\0')))
    return 0;

  return 1;
}

/* Sort the files now in the table.  */

static void
sort_files (void)
{
  int (*func) ();

  switch (sort_type)
    {
    case sort_none:
      return;
    case sort_time:
      switch (time_type)
	{
	case time_ctime:
	  func = sort_reverse ? rev_cmp_ctime : compare_ctime;
	  break;
	case time_mtime:
	  func = sort_reverse ? rev_cmp_mtime : compare_mtime;
	  break;
	case time_atime:
	  func = sort_reverse ? rev_cmp_atime : compare_atime;
	  break;
	default:
	  abort ();
	}
      break;
    case sort_name:
      func = sort_reverse ? rev_cmp_name : compare_name;
      break;
    case sort_extension:
      func = sort_reverse ? rev_cmp_extension : compare_extension;
      break;
    case sort_size:
      func = sort_reverse ? rev_cmp_size : compare_size;
      break;
    default:
      abort ();
    }

  qsort (files, files_index, sizeof (struct fileinfo), func);
}

/* Comparison routines for sorting the files. */

static int
compare_ctime (struct fileinfo *file1, struct fileinfo *file2)
{
  return longdiff (file2->stat.st_ctime, file1->stat.st_ctime);
}

static int
rev_cmp_ctime (struct fileinfo *file2, struct fileinfo *file1)
{
  return longdiff (file2->stat.st_ctime, file1->stat.st_ctime);
}

static int
compare_mtime (struct fileinfo *file1, struct fileinfo *file2)
{
  return longdiff (file2->stat.st_mtime, file1->stat.st_mtime);
}

static int
rev_cmp_mtime (struct fileinfo *file2, struct fileinfo *file1)
{
  return longdiff (file2->stat.st_mtime, file1->stat.st_mtime);
}

static int
compare_atime (struct fileinfo *file1, struct fileinfo *file2)
{
  return longdiff (file2->stat.st_atime, file1->stat.st_atime);
}

static int
rev_cmp_atime (struct fileinfo *file2, struct fileinfo *file1)
{
  return longdiff (file2->stat.st_atime, file1->stat.st_atime);
}

static int
compare_size (struct fileinfo *file1, struct fileinfo *file2)
{
  return longdiff (file2->stat.st_size, file1->stat.st_size);
}

static int
rev_cmp_size (struct fileinfo *file2, struct fileinfo *file1)
{
  return longdiff (file2->stat.st_size, file1->stat.st_size);
}

static int
compare_name (struct fileinfo *file1, struct fileinfo *file2)
{
  return strcmp (file1->name, file2->name);
}

static int
rev_cmp_name (struct fileinfo *file2, struct fileinfo *file1)
{
  return strcmp (file1->name, file2->name);
}

/* Compare file extensions.  Files with no extension are `smallest'.
   If extensions are the same, compare by filenames instead. */

static int
compare_extension (struct fileinfo *file1, struct fileinfo *file2)
{
  register char *base1, *base2;
  register int cmp;

  base1 = strrchr (file1->name, '.');
  base2 = strrchr (file2->name, '.');
  if (base1 == 0 && base2 == 0)
    return strcmp (file1->name, file2->name);
  if (base1 == 0)
    return -1;
  if (base2 == 0)
    return 1;
  cmp = strcmp (base1, base2);
  if (cmp == 0)
    return strcmp (file1->name, file2->name);
  return cmp;
}

static int
rev_cmp_extension (struct fileinfo *file2, struct fileinfo *file1)
{
  register char *base1, *base2;
  register int cmp;

  base1 = strrchr (file1->name, '.');
  base2 = strrchr (file2->name, '.');
  if (base1 == 0 && base2 == 0)
    return strcmp (file1->name, file2->name);
  if (base1 == 0)
    return -1;
  if (base2 == 0)
    return 1;
  cmp = strcmp (base1, base2);
  if (cmp == 0)
    return strcmp (file1->name, file2->name);
  return cmp;
}

/* List all the files now in the table.  */

static void
print_current_files (void)
{
  register int i;

  switch (format)
    {
    case one_per_line:
      for (i = 0; i < files_index; i++)
	{
	  print_file_name_and_frills (files + i);
	  putchar ('\n');
	}
      break;

    case many_per_line:
      print_many_per_line ();
      break;

    case horizontal:
      print_horizontal ();
      break;

    case with_commas:
      print_with_commas ();
      break;

    case long_format:
      for (i = 0; i < files_index; i++)
	{
	  print_long_format (files + i);
	  PUTCHAR ('\n');
	}
      break;
    }
}

static void
print_long_format (struct fileinfo *f)
{
  char modebuf[20];
  char timebuf[40];

  /* 7 fields that may (worst case: 64-bit integral values) require 20 bytes,
     1 10-character mode string,
     1 24-character time string,
     9 spaces, one following each of these fields,
     and 1 trailing NUL byte.  */
  char bigbuf[7 * 20 + 10 + 24 + 9 + 1];
  char *p;
  time_t when;

  mode_string (f->stat.st_mode, modebuf);
  modebuf[10] = '\0';

  switch (time_type)
    {
    case time_ctime:
      when = f->stat.st_ctime;
      break;
    case time_mtime:
      when = f->stat.st_mtime;
      break;
    case time_atime:
      when = f->stat.st_atime;
      break;
    }

  strcpy (timebuf, ctime (&when));

  if (full_time)
    timebuf[24] = '\0';
  else
    {
      if (current_time > when + 6L * 30L * 24L * 60L * 60L	/* Old. */
	  || current_time < when - 60L * 60L)	/* In the future. */
	{
	  /* The file is fairly old or in the future.
	     POSIX says the cutoff is 6 months old;
	     approximate this by 6*30 days.
	     Allow a 1 hour slop factor for what is considered "the future",
	     to allow for NFS server/client clock disagreement.
	     Show the year instead of the time of day.  */
	  strcpy (timebuf + 11, timebuf + 19);
	}
      timebuf[16] = 0;
    }

  p = bigbuf;

  if (print_inode)
    {
      sprintf (p, "%*lu ", INODE_DIGITS, (unsigned long) f->stat.st_ino);
      p += strlen (p);
    }

  if (print_block_size)
    {
      sprintf (p, "%*u ", block_size_size,
	       (unsigned) convert_blocks (ST_NBLOCKS (f->stat),
					  kilobyte_blocks));
      p += strlen (p);
    }

  /* The space between the mode and the number of links is the POSIX
     "optional alternate access method flag". */
  sprintf (p, "%s %3u ", modebuf, (unsigned int) f->stat.st_nlink);
  p += strlen (p);

  if (numeric_users)
    sprintf (p, "%-8u ", (unsigned int) f->stat.st_uid);
  else
    sprintf (p, "%-8.8s ", getuser (f->stat.st_uid));
  p += strlen (p);

  if (!inhibit_group)
    {
      if (numeric_users)
	sprintf (p, "%-8u ", (unsigned int) f->stat.st_gid);
      else
	sprintf (p, "%-8.8s ", getgroup (f->stat.st_gid));
      p += strlen (p);
    }

  if (S_ISCHR (f->stat.st_mode) || S_ISBLK (f->stat.st_mode))
    sprintf (p, "%3u, %3u ", (unsigned) major (f->stat.st_rdev),
	     (unsigned) minor (f->stat.st_rdev));
  else
    sprintf (p, "%8lu ", (unsigned long) f->stat.st_size);
  p += strlen (p);

  sprintf (p, "%s ", full_time ? timebuf : timebuf + 4);
  p += strlen (p);

  DIRED_INDENT ();
  FPUTS (bigbuf, stdout, p - bigbuf);
  PUSH_CURRENT_DIRED_POS (&dired_obstack);
  print_name_with_quoting (f->name);
  PUSH_CURRENT_DIRED_POS (&dired_obstack);

  if (f->filetype == symbolic_link)
    {
      if (f->linkname)
	{
	  FPUTS_LITERAL (" -> ", stdout);
	  print_name_with_quoting (f->linkname);
	  if (indicator_style != none)
	    print_type_indicator (f->linkmode);
	}
    }
  else if (indicator_style != none)
    print_type_indicator (f->stat.st_mode);
}

/* Set QUOTED_LENGTH to strlen(P) and return NULL if P == quoted(P).
   Otherwise, return xmalloc'd storage containing the quoted version
   of P and set QUOTED_LENGTH to the length of the quoted P.  */

static char *
quote_filename (register const char *p, size_t *quoted_length)
{
  register unsigned char c;
  const char *p0 = p;
  char *quoted, *q;
  int found_quotable;

  if (!quote_as_string && !quote_funny_chars && !qmark_funny_chars)
    {
      *quoted_length = strlen (p);
      return NULL;
    }

  found_quotable = 0;
  for (c = *p; c; c = *++p)
    {
      if (quote_funny_chars)
	{
	  switch (c)
	    {
	    case '\\':
	    case '\n':
	    case '\b':
	    case '\r':
	    case '\t':
	    case '\f':
	    case ' ':
	    case '"':
	      found_quotable = 1;
	      break;

	    default:
	      if (!ISGRAPH (c))
		found_quotable = 1;
	      break;
	    }
	}
      else
	{
	  if (!ISPRINT (c) && qmark_funny_chars)
	    found_quotable = 1;
	}
      if (found_quotable)
	break;
    }

  if (!found_quotable && !quote_as_string)
    {
      *quoted_length = p - p0;
      return NULL;
    }

  p = p0;
  quoted = xmalloc (4 * strlen (p) + 1);
  q = quoted;

#define SAVECHAR(c) *q++ = (c)
#define SAVE_2_CHARS(c12) \
    do { *q++ = ((c12)[0]); \
	 *q++ = ((c12)[1]); } while (0)

  if (quote_as_string)
    SAVECHAR ('"');

  while ((c = *p++))
    {
      if (quote_funny_chars)
	{
	  switch (c)
	    {
	    case '\\':
	      SAVE_2_CHARS ("\\\\");
	      break;

	    case '\n':
	      SAVE_2_CHARS ("\\n");
	      break;

	    case '\b':
	      SAVE_2_CHARS ("\\b");
	      break;

	    case '\r':
	      SAVE_2_CHARS ("\\r");
	      break;

	    case '\t':
	      SAVE_2_CHARS ("\\t");
	      break;

	    case '\f':
	      SAVE_2_CHARS ("\\f");
	      break;

	    case ' ':
	      SAVE_2_CHARS ("\\ ");
	      break;

	    case '"':
	      SAVE_2_CHARS ("\\\"");
	      break;

	    default:
	      if (ISGRAPH (c))
		SAVECHAR (c);
	      else
		{
		  char buf[5];
		  sprintf (buf, "\\%03o", (unsigned int) c);
		  q = stpcpy (q, buf);
		}
	    }
	}
      else
	{
	  if (ISPRINT (c))
	    SAVECHAR (c);
	  else if (!qmark_funny_chars)
	    SAVECHAR (c);
	  else
	    SAVECHAR ('?');
	}
    }

  if (quote_as_string)
    SAVECHAR ('"');

  *quoted_length = q - quoted;

  SAVECHAR ('\0');

  return quoted;
}

static void
print_name_with_quoting (register char *p)
{
  char *quoted;
  size_t quoted_length;

  quoted = quote_filename (p, &quoted_length);
  FPUTS (quoted != NULL ? quoted : p, stdout, quoted_length);
  if (quoted)
    free (quoted);
}

/* Print the file name of `f' with appropriate quoting.
   Also print file size, inode number, and filetype indicator character,
   as requested by switches.  */

static void
print_file_name_and_frills (struct fileinfo *f)
{
  if (print_inode)
    printf ("%*lu ", INODE_DIGITS, (unsigned long) f->stat.st_ino);

  if (print_block_size)
    printf ("%*u ", block_size_size,
	    (unsigned) convert_blocks (ST_NBLOCKS (f->stat),
				       kilobyte_blocks));

  print_name_with_quoting (f->name);

  if (indicator_style != none)
    print_type_indicator (f->stat.st_mode);
}

static void
print_type_indicator (unsigned int mode)
{
  if (S_ISDIR (mode))
    PUTCHAR ('/');

#ifdef S_ISLNK
  if (S_ISLNK (mode))
    PUTCHAR ('@');
#endif

#ifdef S_ISFIFO
  if (S_ISFIFO (mode))
    PUTCHAR ('|');
#endif

#ifdef S_ISSOCK
  if (S_ISSOCK (mode))
    PUTCHAR ('=');
#endif

  if (S_ISREG (mode) && indicator_style == all
      && (mode & (S_IEXEC | S_IXGRP | S_IXOTH)))
    PUTCHAR ('*');
}

static int
length_of_file_name_and_frills (struct fileinfo *f)
{
  register char *p = f->name;
  register unsigned char c;
  register int len = 0;

  if (print_inode)
    len += INODE_DIGITS + 1;

  if (print_block_size)
    len += 1 + block_size_size;

  if (quote_as_string)
    len += 2;

  while ((c = *p++))
    {
      if (quote_funny_chars)
	{
	  switch (c)
	    {
	    case '\\':
	    case '\n':
	    case '\b':
	    case '\r':
	    case '\t':
	    case '\f':
	    case ' ':
	      len += 2;
	      break;

	    case '"':
	      if (quote_as_string)
		len += 2;
	      else
		len += 1;
	      break;

	    default:
	      if (ISPRINT (c))
		len += 1;
	      else
		len += 4;
	    }
	}
      else
	len += 1;
    }

  if (indicator_style != none)
    {
      unsigned filetype = f->stat.st_mode;

      if (S_ISREG (filetype))
	{
	  if (indicator_style == all
	      && (f->stat.st_mode & (S_IEXEC | S_IEXEC >> 3 | S_IEXEC >> 6)))
	    len += 1;
	}
      else if (S_ISDIR (filetype)
#ifdef S_ISLNK
	       || S_ISLNK (filetype)
#endif
#ifdef S_ISFIFO
	       || S_ISFIFO (filetype)
#endif
#ifdef S_ISSOCK
	       || S_ISSOCK (filetype)
#endif
	)
	len += 1;
    }

  return len;
}

static void
print_many_per_line (void)
{
  int filesno;			/* Index into files. */
  int row;			/* Current row. */
  int max_name_length;		/* Length of longest file name + frills. */
  int name_length;		/* Length of each file name + frills. */
  int pos;			/* Current character column. */
  int cols;			/* Number of files across. */
  int rows;			/* Maximum number of files down. */

  /* Compute the maximum file name length.  */
  max_name_length = 0;
  for (filesno = 0; filesno < files_index; filesno++)
    {
      name_length = length_of_file_name_and_frills (files + filesno);
      if (name_length > max_name_length)
	max_name_length = name_length;
    }

  /* Allow at least two spaces between names.  */
  max_name_length += 2;

  /* Calculate the maximum number of columns that will fit. */
  cols = line_length / max_name_length;
  if (cols == 0)
    cols = 1;
  /* Calculate the number of rows that will be in each column except possibly
     for a short column on the right. */
  rows = files_index / cols + (files_index % cols != 0);
  /* Recalculate columns based on rows. */
  cols = files_index / rows + (files_index % rows != 0);

  for (row = 0; row < rows; row++)
    {
      filesno = row;
      pos = 0;
      /* Print the next row.  */
      while (1)
	{
	  print_file_name_and_frills (files + filesno);
	  name_length = length_of_file_name_and_frills (files + filesno);

	  filesno += rows;
	  if (filesno >= files_index)
	    break;

	  indent (pos + name_length, pos + max_name_length);
	  pos += max_name_length;
	}
      putchar ('\n');
    }
}

static void
print_horizontal (void)
{
  int filesno;
  int max_name_length;
  int name_length;
  int cols;
  int pos;

  /* Compute the maximum file name length.  */
  max_name_length = 0;
  for (filesno = 0; filesno < files_index; filesno++)
    {
      name_length = length_of_file_name_and_frills (files + filesno);
      if (name_length > max_name_length)
	max_name_length = name_length;
    }

  /* Allow two spaces between names.  */
  max_name_length += 2;

  cols = line_length / max_name_length;
  if (cols == 0)
    cols = 1;

  pos = 0;
  name_length = 0;

  for (filesno = 0; filesno < files_index; filesno++)
    {
      if (filesno != 0)
	{
	  if (filesno % cols == 0)
	    {
	      putchar ('\n');
	      pos = 0;
	    }
	  else
	    {
	      indent (pos + name_length, pos + max_name_length);
	      pos += max_name_length;
	    }
	}

      print_file_name_and_frills (files + filesno);

      name_length = length_of_file_name_and_frills (files + filesno);
    }
  putchar ('\n');
}

static void
print_with_commas (void)
{
  int filesno;
  int pos, old_pos;

  pos = 0;

  for (filesno = 0; filesno < files_index; filesno++)
    {
      old_pos = pos;

      pos += length_of_file_name_and_frills (files + filesno);
      if (filesno + 1 < files_index)
	pos += 2;		/* For the comma and space */

      if (old_pos != 0 && pos >= line_length)
	{
	  putchar ('\n');
	  pos -= old_pos;
	}

      print_file_name_and_frills (files + filesno);
      if (filesno + 1 < files_index)
	{
	  putchar (',');
	  putchar (' ');
	}
    }
  putchar ('\n');
}

/* Assuming cursor is at position FROM, indent up to position TO.
   Use a TAB character instead of two or more spaces whenever possible.  */

static void
indent (int from, int to)
{
  while (from < to)
    {
      if (to / tabsize > (from + 1) / tabsize)
	{
	  putchar ('\t');
	  from += tabsize - from % tabsize;
	}
      else
	{
	  putchar (' ');
	  from++;
	}
    }
}

/* Put DIRNAME/NAME into DEST, handling `.' and `/' properly. */

static void
attach (char *dest, const char *dirname, const char *name)
{
  const char *dirnamep = dirname;

  /* Copy dirname if it is not ".". */
  if (dirname[0] != '.' || dirname[1] != 0)
    {
      while (*dirnamep)
	*dest++ = *dirnamep++;
      /* Add '/' if `dirname' doesn't already end with it. */
      if (dirnamep > dirname && dirnamep[-1] != '/')
	*dest++ = '/';
    }
  while (*name)
    *dest++ = *name++;
  *dest = 0;
}

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
      printf (_("\
List information about the FILEs (the current directory by default).\n\
Sort entries alphabetically if none of -cftuSUX nor --sort.\n\
\n\
  -A, --almost-all           do not list implied . and ..\n\
  -a, --all                  do not hide entries starting with .\n\
  -B, --ignore-backups       do not list implied entries ending with ~\n\
  -b, --escape               print octal escapes for nongraphic characters\n\
  -C                         list entries by columns\n\
  -c                         sort by change time; with -l: show ctime\n\
  -D, --dired                generate output well suited to Emacs' dired mode\n\
  -d, --directory            list directory entries instead of contents\n\
  -F, --classify             append a character for typing each entry\n\
  -f                         do not sort, enable -aU, disable -lst\n\
      --format=WORD          across -x, commas -m, horizontal -x, long -l,\n\
                               single-column -1, verbose -l, vertical -C\n\
      --full-time            list both full date and full time\n"));

      printf (_("\
  -G, --no-group             inhibit display of group information\n\
  -g                         (ignored)\n\
  -I, --ignore=PATTERN       do not list implied entries matching shell PATTERN\n\
  -i, --inode                print index number of each file\n\
  -k, --kilobytes            use 1024 blocks, not 512 despite POSIXLY_CORRECT\n\
  -L, --dereference          list entries pointed to by symbolic links\n\
  -l                         use a long listing format\n\
  -m                         fill width with a comma separated list of entries\n\
  -N, --literal              do not quote entry names\n\
  -n, --numeric-uid-gid      list numeric UIDs and GIDs instead of names\n\
  -o                         use long listing format without group info\n\
  -p                         append a character for typing each entry\n\
  -Q, --quote-name           enclose entry names in double quotes\n\
  -q, --hide-control-chars   print ? instead of non graphic characters\n\
  -R, --recursive            list subdirectories recursively\n\
  -r, --reverse              reverse order while sorting\n\
  -S                         sort by file size\n"));

      printf (_("\
  -s, --size                 print block size of each file\n\
      --sort=WORD            ctime -c, extension -X, none -U, size -S,\n\
                               status -c, time -t\n\
      --time=WORD            atime -u, access -u, use -u\n\
  -T, --tabsize=COLS         assume tab stops at each COLS instead of 8\n\
  -t                         sort by modification time; with -l: show mtime\n\
  -U                         do not sort; list entries in directory order\n\
  -u                         sort by last access time; with -l: show atime\n\
  -w, --width=COLS           assume screen width instead of current value\n\
  -x                         list entries by lines instead of by columns\n\
  -X                         sort alphabetically by entry extension\n\
  -1                         list one file per line\n\
      --help                 display this help and exit\n\
      --version              output version information and exit"));
    }
  exit (status);
}
