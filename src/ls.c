/* `dir', `vdir' and `ls' directory listing programs for GNU.
   Copyright (C) 85, 88, 90, 91, 1995-2000 Free Software Foundation, Inc.

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

/* If ls_mode is LS_MULTI_COL,
   the multi-column format is the default regardless
   of the type of output device.
   This is for the `dir' program.

   If ls_mode is LS_LONG_FORMAT,
   the long format is the default regardless of the
   type of output device.
   This is for the `vdir' program.

   If ls_mode is LS_LS,
   the output format depends on whether the output
   device is a terminal.
   This is for the `ls' program. */

/* Written by Richard Stallman and David MacKenzie.  */

/* Color support by Peter Anvin <Peter.Anvin@linux.org> and Dennis
   Flaherty <dennisf@denix.elk.miles.com> based on original patches by
   Greg Lee <lee@uhunix.uhcc.hawaii.edu>.  */

#ifdef _AIX
 #pragma alloca
#endif

#include <config.h>
#include <sys/types.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#if HAVE_TERMIOS_H
# include <termios.h>
#endif

#ifdef GWINSZ_IN_SYS_IOCTL
# include <sys/ioctl.h>
#endif

#ifdef WINSIZE_IN_PTEM
# include <sys/stream.h>
# include <sys/ptem.h>
#endif

#if HAVE_SYS_ACL_H
# include <sys/acl.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <getopt.h>

/* Get MB_LEN_MAX.  */
#if HAVE_LIMITS_H
# include <limits.h>
#endif

/* Get MB_CUR_MAX.  */
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

/* Get mbstate_t, mbrtowc(), mbsinit(), wcwidth().  */
#if HAVE_WCHAR_H
# include <wchar.h>
#endif

/* Get iswprint().  */
#if HAVE_WCTYPE_H
# include <wctype.h>
#endif
#if !defined iswprint && !HAVE_ISWPRINT
# define iswprint(wc) 1
#endif

/* Some systems, like BeOS, have multibyte encodings but lack mbstate_t.  */
#if HAVE_MBRTOWC && defined mbstate_t
# define mbrtowc(pwc, s, n, ps) (mbrtowc) (pwc, s, n, 0)
# define mbsinit(ps) 1
#endif

#ifndef HAVE_DECL_WCWIDTH
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_WCWIDTH
int wcwidth ();
#endif

/* If wcwidth() doesn't exist, assume all printable characters have
   width 1.  */
#ifndef wcwidth
# if !HAVE_WCWIDTH
#  define wcwidth(wc) ((wc) == 0 ? 0 : iswprint (wc) ? 1 : -1)
# endif
#endif

#include "system.h"
#include <fnmatch.h>

#include "argmatch.h"
#include "error.h"
#include "human.h"
#include "filemode.h"
#include "ls.h"
#include "mbswidth.h"
#include "obstack.h"
#include "path-concat.h"
#include "quotearg.h"
#include "strverscmp.h"
#include "xstrtol.h"

/* Use access control lists only under all the following conditions.
   Some systems (OSF4, Irix5, Irix6) have the acl function, but not
   sys/acl.h or don't define the GETACLCNT macro.  */
#if HAVE_SYS_ACL_H && HAVE_ACL && defined GETACLCNT
# define USE_ACL 1
#endif

#define PROGRAM_NAME (ls_mode == LS_LS ? "ls" \
		      : (ls_mode == LS_MULTI_COL \
			 ? "dir" : "vdir"))

#define AUTHORS "Richard Stallman and David MacKenzie"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

/* Return an int indicating the result of comparing two integers.
   Subtracting doesn't always work, due to overflow.  */
#define longdiff(a, b) ((a) < (b) ? -1 : (a) > (b))

/* The field width for inode numbers.  On some hosts inode numbers are
   64 bits, so columns won't line up exactly when a huge inode number
   is encountered, but in practice 7 digits is usually enough.  */
#ifndef INODE_DIGITS
# define INODE_DIGITS 7
#endif

#ifdef S_ISLNK
# define HAVE_SYMLINKS 1
#else
# define HAVE_SYMLINKS 0
#endif

/* If any of the S_* macros are undefined, define them here so each
   use doesn't have to be guarded with e.g., #ifdef S_ISLNK.  */
#ifndef S_ISLNK
# define S_ISLNK(Mode) 0
#endif

#ifndef S_ISFIFO
# define S_ISFIFO(Mode) 0
#endif

#ifndef S_ISSOCK
# define S_ISSOCK(Mode) 0
#endif

#ifndef S_ISCHR
# define S_ISCHR(Mode) 0
#endif

#ifndef S_ISBLK
# define S_ISBLK(Mode) 0
#endif

#ifndef S_ISDOOR
# define S_ISDOOR(Mode) 0
#endif

/* Arrange to make lstat calls go through the wrapper function
   on systems with an lstat function that does not dereference symlinks
   that are specified with a trailing slash.  */
#if ! LSTAT_FOLLOWS_SLASHED_SYMLINK
int rpl_lstat PARAMS((const char *, struct stat *));
# undef lstat
# define lstat(Name, Stat_buf) rpl_lstat(Name, Stat_buf)
#endif

#if defined _DIRENT_HAVE_D_TYPE || defined DTTOIF
# define HAVE_STRUCT_DIRENT_D_TYPE 1
# define DT_INIT(Val) = Val
#else
# define HAVE_STRUCT_DIRENT_D_TYPE 0
# define DT_INIT(Val) /* empty */
#endif

enum filetype
  {
    unknown DT_INIT (DT_UNKNOWN),
    fifo DT_INIT (DT_FIFO),
    chardev DT_INIT (DT_CHR),
    directory DT_INIT (DT_DIR),
    blockdev DT_INIT (DT_BLK),
    normal DT_INIT (DT_REG),
    symbolic_link DT_INIT (DT_LNK),
    sock DT_INIT (DT_SOCK),
    arg_directory DT_INIT (2 * (DT_UNKNOWN | DT_FIFO | DT_CHR | DT_DIR | DT_BLK
				| DT_REG | DT_LNK | DT_SOCK))
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

    /* For symbolic link and color printing, 1 if linked-to file
       exists, otherwise 0.  */
    int linkok;

    enum filetype filetype;

#if USE_ACL
    /* For long listings, nonzero if the file has an access control list,
       otherwise zero.  */
    int have_acl;
#endif
  };

#if USE_ACL
# define FILE_HAS_ACL(F) ((F)->have_acl)
#else
# define FILE_HAS_ACL(F) 0
#endif

#define LEN_STR_PAIR(s) sizeof (s) - 1, s

/* Null is a valid character in a color indicator (think about Epson
   printers, for example) so we have to use a length/buffer string
   type.  */

struct bin_str
  {
    int len;			/* Number of bytes */
    const char *string;		/* Pointer to the same */
  };

#ifndef STDC_HEADERS
time_t time ();
#endif

char *getgroup ();
char *getuser ();

static size_t quote_name PARAMS ((FILE *out, const char *name,
				  struct quoting_options const *options));
static char *make_link_path PARAMS ((const char *path, const char *linkname));
static int compare_atime PARAMS ((const struct fileinfo *file1,
				  const struct fileinfo *file2));
static int rev_cmp_atime PARAMS ((const struct fileinfo *file2,
				  const struct fileinfo *file1));
static int compare_ctime PARAMS ((const struct fileinfo *file1,
				  const struct fileinfo *file2));
static int rev_cmp_ctime PARAMS ((const struct fileinfo *file2,
				  const struct fileinfo *file1));
static int compare_mtime PARAMS ((const struct fileinfo *file1,
				  const struct fileinfo *file2));
static int rev_cmp_mtime PARAMS ((const struct fileinfo *file2,
				  const struct fileinfo *file1));
static int compare_size PARAMS ((const struct fileinfo *file1,
				 const struct fileinfo *file2));
static int rev_cmp_size PARAMS ((const struct fileinfo *file2,
				 const struct fileinfo *file1));
static int compare_name PARAMS ((const struct fileinfo *file1,
				 const struct fileinfo *file2));
static int rev_cmp_name PARAMS ((const struct fileinfo *file2,
				 const struct fileinfo *file1));
static int compare_extension PARAMS ((const struct fileinfo *file1,
				      const struct fileinfo *file2));
static int rev_cmp_extension PARAMS ((const struct fileinfo *file2,
				      const struct fileinfo *file1));
static int compare_version PARAMS ((const struct fileinfo *file1,
				    const struct fileinfo *file2));
static int rev_cmp_version PARAMS ((const struct fileinfo *file2,
				    const struct fileinfo *file1));
static int decode_switches PARAMS ((int argc, char **argv));
static int file_interesting PARAMS ((const struct dirent *next));
static uintmax_t gobble_file PARAMS ((const char *name, enum filetype type,
				      int explicit_arg, const char *dirname));
static void print_color_indicator PARAMS ((const char *name, unsigned int mode,
					   int linkok));
static void put_indicator PARAMS ((const struct bin_str *ind));
static int length_of_file_name_and_frills PARAMS ((const struct fileinfo *f));
static void add_ignore_pattern PARAMS ((const char *pattern));
static void attach PARAMS ((char *dest, const char *dirname, const char *name));
static void clear_files PARAMS ((void));
static void extract_dirs_from_files PARAMS ((const char *dirname,
					     int recursive));
static void get_link_name PARAMS ((const char *filename, struct fileinfo *f));
static void indent PARAMS ((int from, int to));
static void init_column_info PARAMS ((void));
static void print_current_files PARAMS ((void));
static void print_dir PARAMS ((const char *name, const char *realname));
static void print_file_name_and_frills PARAMS ((const struct fileinfo *f));
static void print_horizontal PARAMS ((void));
static void print_long_format PARAMS ((const struct fileinfo *f));
static void print_many_per_line PARAMS ((void));
static void print_name_with_quoting PARAMS ((const char *p, unsigned int mode,
					     int linkok,
					     struct obstack *stack));
static void prep_non_filename_text PARAMS ((void));
static void print_type_indicator PARAMS ((unsigned int mode));
static void print_with_commas PARAMS ((void));
static void queue_directory PARAMS ((const char *name, const char *realname));
static void sort_files PARAMS ((void));
static void parse_ls_color PARAMS ((void));
void usage PARAMS ((int status));

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

/* When nonzero, in a color listing, color each symlink name according to the
   type of file it points to.  Otherwise, color them according to the `ln'
   directive in LS_COLORS.  Dangling (orphan) symlinks are treated specially,
   regardless.  This is set when `ln=target' appears in LS_COLORS.  */

static int color_symlink_as_referent;

/* mode of appropriate file for colorization */
#define FILE_OR_LINK_MODE(File) \
    ((color_symlink_as_referent && (File)->linkok) \
     ? (File)->linkmode : (File)->stat.st_mode)


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

static int full_time;

/* The file characteristic to sort by.  Controlled by -t, -S, -U, -X, -v. */

enum sort_type
  {
    sort_none,			/* -U */
    sort_name,			/* default */
    sort_extension,		/* -X */
    sort_time,			/* -t */
    sort_size,			/* -S */
    sort_version		/* -v */
  };

static enum sort_type sort_type;

/* Direction of sort.
   0 means highest first if numeric,
   lowest first if alphabetic;
   these are the defaults.
   1 means the opposite order in each case.  -r  */

static int sort_reverse;

/* Nonzero means to NOT display group information.  -G  */

static int inhibit_group;

/* Nonzero means print the user and group id's as numbers rather
   than as names.  -n  */

static int numeric_ids;

/* Nonzero means mention the size in blocks of each file.  -s  */

static int print_block_size;

/* If positive, the units to use when printing sizes;
   if negative, the human-readable base.  */
static int output_block_size;

/* Precede each line of long output (per file) with a string like `m,n:'
   where M is the number of characters after the `:' and before the
   filename and N is the length of the filename.  Using this format,
   Emacs' dired mode starts up twice as fast, and can handle all
   strange characters in file names.  */
static int dired;

/* `none' means don't mention the type of files.
   `classify' means mention file types and mark executables.
   `file_type' means mention only file types.

   Controlled by -F, -p, and --indicator-style.  */

enum indicator_style
  {
    none,	/*     --indicator-style=none */
    classify,	/* -F, --indicator-style=classify */
    file_type	/* -p, --indicator-style=file-type */
  };

static enum indicator_style indicator_style;

/* Names of indicator styles.  */
static char const *const indicator_style_args[] =
{
  "none", "classify", "file-type", 0
};

static enum indicator_style const indicator_style_types[]=
{
  none, classify, file_type
};

/* Nonzero means use colors to mark types.  Also define the different
   colors as well as the stuff for the LS_COLORS environment variable.
   The LS_COLORS variable is now in a termcap-like format.  */

static int print_with_color;

enum color_type
  {
    color_never,		/* 0: default or --color=never */
    color_always,		/* 1: --color=always */
    color_if_tty		/* 2: --color=tty */
  };

enum indicator_no
  {
    C_LEFT, C_RIGHT, C_END, C_NORM, C_FILE, C_DIR, C_LINK, C_FIFO, C_SOCK,
    C_BLK, C_CHR, C_MISSING, C_ORPHAN, C_EXEC, C_DOOR
  };

static const char *const indicator_name[]=
  {
    "lc", "rc", "ec", "no", "fi", "di", "ln", "pi", "so",
    "bd", "cd", "mi", "or", "ex", "do", NULL
  };

struct color_ext_type
  {
    struct bin_str ext;		/* The extension we're looking for */
    struct bin_str seq;		/* The sequence to output when we do */
    struct color_ext_type *next;	/* Next in list */
  };

static struct bin_str color_indicator[] =
  {
    { LEN_STR_PAIR ("\033[") },		/* lc: Left of color sequence */
    { LEN_STR_PAIR ("m") },		/* rc: Right of color sequence */
    { 0, NULL },			/* ec: End color (replaces lc+no+rc) */
    { LEN_STR_PAIR ("0") },		/* no: Normal */
    { LEN_STR_PAIR ("0") },		/* fi: File: default */
    { LEN_STR_PAIR ("01;34") },		/* di: Directory: bright blue */
    { LEN_STR_PAIR ("01;36") },		/* ln: Symlink: bright cyan */
    { LEN_STR_PAIR ("33") },		/* pi: Pipe: yellow/brown */
    { LEN_STR_PAIR ("01;35") },		/* so: Socket: bright magenta */
    { LEN_STR_PAIR ("01;33") },		/* bd: Block device: bright yellow */
    { LEN_STR_PAIR ("01;33") },		/* cd: Char device: bright yellow */
    { 0, NULL },			/* mi: Missing file: undefined */
    { 0, NULL },			/* or: Orphanned symlink: undefined */
    { LEN_STR_PAIR ("01;32") },		/* ex: Executable: bright green */
    { LEN_STR_PAIR ("01;35") }		/* do: Door: bright magenta */
  };

/* FIXME: comment  */
static struct color_ext_type *color_ext_list = NULL;

/* Buffer for color sequences */
static char *color_buf;

/* Nonzero means to check for orphaned symbolic link, for displaying
   colors.  */

static int check_symlink_color;

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
    const char *pattern;
    struct ignore_pattern *next;
  };

static struct ignore_pattern *ignore_patterns;

/* Nonzero means output nongraphic chars in file names as `?'.
   (-q, --hide-control-chars)
   qmark_funny_chars and the quoting style (-Q, --quoting-style=WORD) are
   independent.  The algorithm is: first, obey the quoting style to get a
   string representing the file name;  then, if qmark_funny_chars is set,
   replace all nonprintable chars in that string with `?'.  It's necessary
   to replace nonprintable chars even in quoted strings, because we don't
   want to mess up the terminal if control chars get sent to it, and some
   quoting methods pass through control chars as-is.  */
static int qmark_funny_chars;

/* Quoting options for file and dir name output.  */

static struct quoting_options *filename_quoting_options;
static struct quoting_options *dirname_quoting_options;

/* The number of chars per hardware tab stop.  Setting this to zero
   inhibits the use of TAB characters for separating columns.  -T */
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

/* Similar to `format_needs_stat', but set if only the file type is
   needed.  */

static int format_needs_type;

/* The exit status to use if we don't get any fatal errors. */

static int exit_status;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  BLOCK_SIZE_OPTION = CHAR_MAX + 1,
  COLOR_OPTION,
  FORMAT_OPTION,
  INDICATOR_STYLE_OPTION,
  QUOTING_STYLE_OPTION,
  SHOW_CONTROL_CHARS_OPTION,
  SORT_OPTION,
  TIME_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, 0, 'a'},
  {"escape", no_argument, 0, 'b'},
  {"directory", no_argument, 0, 'd'},
  {"dired", no_argument, 0, 'D'},
  {"full-time", no_argument, &full_time, 1},
  {"human-readable", no_argument, 0, 'h'},
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
  {"file-type", no_argument, 0, 'p'},
  {"si", no_argument, 0, 'H'},
  {"ignore", required_argument, 0, 'I'},
  {"indicator-style", required_argument, 0, INDICATOR_STYLE_OPTION},
  {"dereference", no_argument, 0, 'L'},
  {"literal", no_argument, 0, 'N'},
  {"quote-name", no_argument, 0, 'Q'},
  {"quoting-style", required_argument, 0, QUOTING_STYLE_OPTION},
  {"recursive", no_argument, 0, 'R'},
  {"format", required_argument, 0, FORMAT_OPTION},
  {"show-control-chars", no_argument, 0, SHOW_CONTROL_CHARS_OPTION},
  {"sort", required_argument, 0, SORT_OPTION},
  {"tabsize", required_argument, 0, 'T'},
  {"time", required_argument, 0, TIME_OPTION},
  {"color", optional_argument, 0, COLOR_OPTION},
  {"block-size", required_argument, 0, BLOCK_SIZE_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static char const *const format_args[] =
{
  "verbose", "long", "commas", "horizontal", "across",
  "vertical", "single-column", 0
};

static enum format const format_types[] =
{
  long_format, long_format, with_commas, horizontal, horizontal,
  many_per_line, one_per_line
};

static char const *const sort_args[] =
{
  "none", "time", "size", "extension", "version", 0
};

static enum sort_type const sort_types[] =
{
  sort_none, sort_time, sort_size, sort_extension, sort_version
};

static char const *const time_args[] =
{
  "atime", "access", "use", "ctime", "status", 0
};

static enum time_type const time_types[] =
{
  time_atime, time_atime, time_atime, time_ctime, time_ctime
};

static char const *const color_args[] =
{
  /* force and none are for compatibility with another color-ls version */
  "always", "yes", "force",
  "never", "no", "none",
  "auto", "tty", "if-tty", 0
};

static enum color_type const color_types[] =
{
  color_always, color_always, color_always,
  color_never, color_never, color_never,
  color_if_tty, color_if_tty, color_if_tty
};

/* Information about filling a column.  */
struct column_info
{
  int valid_len;
  int line_len;
  int *col_arr;
};

/* Array with information about column filledness.  */
static struct column_info *column_info;

/* Maximum number of columns ever possible for this display.  */
static int max_idx;

/* The minimum width of a colum is 3: 1 character for the name and 2
   for the separating white space.  */
#define MIN_COLUMN_WIDTH	3


/* This zero-based index is used solely with the --dired option.
   When that option is in effect, this counter is incremented for each
   character of output generated by this program so that the beginning
   and ending indices (in that output) of every file name can be recorded
   and later output themselves.  */
static size_t dired_pos;

#define DIRED_PUTCHAR(c) do {putchar ((c)); ++dired_pos;} while (0)

/* Write S to STREAM and increment DIRED_POS by S_LEN.  */
#define DIRED_FPUTS(s, stream, s_len) \
    do {fputs ((s), (stream)); dired_pos += s_len;} while (0)

/* Like DIRED_FPUTS, but for use when S is a literal string.  */
#define DIRED_FPUTS_LITERAL(s, stream) \
    do {fputs ((s), (stream)); dired_pos += sizeof((s)) - 1;} while (0)

#define DIRED_INDENT()							\
    do									\
      {									\
	/* FIXME: remove the `&& format == long_format' clause.  */	\
	if (dired && format == long_format)				\
	  DIRED_FPUTS_LITERAL ("  ", stdout);				\
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


/* Write to standard output PREFIX, followed by the quoting style and
   a space-separated list of the integers stored in OS all on one line.  */

static void
dired_dump_obstack (const char *prefix, struct obstack *os)
{
  int n_pos;

  n_pos = obstack_object_size (os) / sizeof (dired_pos);
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

  atexit (close_stdout);

#define N_ENTRIES(Array) (sizeof Array / sizeof *(Array))
  assert (N_ENTRIES (color_indicator) + 1 == N_ENTRIES (indicator_name));

  exit_status = 0;
  dir_defaulted = 1;
  print_dir_name = 1;
  pending_dirs = 0;
  current_time = time ((time_t *) 0);

  i = decode_switches (argc, argv);

  if (print_with_color)
    {
      parse_ls_color ();
      prep_non_filename_text ();
      /* Avoid following symbolic links when possible.  */
      if (color_indicator[C_ORPHAN].string != NULL
	  || (color_indicator[C_MISSING].string != NULL
	      && format == long_format))
	check_symlink_color = 1;
    }

  format_needs_stat = sort_type == sort_time || sort_type == sort_size
    || format == long_format
    || trace_links || trace_dirs || print_block_size || print_inode;
  format_needs_type = (format_needs_stat == 0
		       && (print_with_color || indicator_style != none));

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
    {
      gobble_file (argv[i], unknown, 1, "");
    }

  if (dir_defaulted)
    {
      if (immediate_dirs)
	gobble_file (".", directory, 1, "");
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
	DIRED_PUTCHAR ('\n');
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
      printf ("//DIRED-OPTIONS// --quoting-style=%s\n",
	      ARGMATCH_TO_ARGUMENT (filename_quoting_options,
				    quoting_style_args, quoting_style_vals));
    }

  /* Restore default color before exiting */
  if (print_with_color)
    {
      put_indicator (&color_indicator[C_LEFT]);
      put_indicator (&color_indicator[C_RIGHT]);
    }

  exit (exit_status);
}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */

static int
decode_switches (int argc, char **argv)
{
  register char const *p;
  int c;
  int i;
  long int tmp_long;

  /* Record whether there is an option specifying sort type.  */
  int sort_type_specified = 0;

  qmark_funny_chars = 0;

  /* initialize all switches to default settings */

  switch (ls_mode)
    {
    case LS_MULTI_COL:
      /* This is for the `dir' program.  */
      format = many_per_line;
      set_quoting_style (NULL, escape_quoting_style);
      break;

    case LS_LONG_FORMAT:
      /* This is for the `vdir' program.  */
      format = long_format;
      set_quoting_style (NULL, escape_quoting_style);
      break;

    case LS_LS:
      /* This is for the `ls' program.  */
      if (isatty (STDOUT_FILENO))
	{
	  format = many_per_line;
	  /* See description of qmark_funny_chars, above.  */
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
  numeric_ids = 0;
  print_block_size = 0;
  indicator_style = none;
  print_inode = 0;
  trace_links = 0;
  trace_dirs = 0;
  immediate_dirs = 0;
  all_files = 0;
  really_all_files = 0;
  ignore_patterns = 0;

  /* FIXME: Shouldn't we complain on wrong values? */
  if ((p = getenv ("QUOTING_STYLE"))
      && 0 <= (i = ARGCASEMATCH (p, quoting_style_args, quoting_style_vals)))
    set_quoting_style (NULL, quoting_style_vals[i]);

  human_block_size (getenv ("LS_BLOCK_SIZE"), 0, &output_block_size);

  line_length = 80;
  if ((p = getenv ("COLUMNS")) && *p)
    {
      if (xstrtol (p, NULL, 0, &tmp_long, NULL) == LONGINT_OK
	  && 0 < tmp_long && tmp_long <= INT_MAX)
	{
	  line_length = (int) tmp_long;
	}
      else
	{
	  error (0, 0,
	       _("ignoring invalid width in environment variable COLUMNS: %s"),
		 quotearg (p));
	}
    }

#ifdef TIOCGWINSZ
  {
    struct winsize ws;

    if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0)
      line_length = ws.ws_col;
  }
#endif

  /* Using the TABSIZE environment variable is not POSIX-approved.
     Ignore it when POSIXLY_CORRECT is set.  */
  tabsize = 8;
  if (!getenv ("POSIXLY_CORRECT") && (p = getenv ("TABSIZE")))
    {
      if (xstrtol (p, NULL, 0, &tmp_long, NULL) == LONGINT_OK
	  && 0 <= tmp_long && tmp_long <= INT_MAX)
	{
	  tabsize = (int) tmp_long;
	}
      else
	{
	  error (0, 0,
	   _("ignoring invalid tab size in environment variable TABSIZE: %s"),
		 quotearg (p));
	}
    }

  while ((c = getopt_long (argc, argv,
			   "abcdfghiklmnopqrstuvw:xABCDFGHI:LNQRST:UX1",
			   long_options, NULL)) != -1)
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
	  set_quoting_style (NULL, escape_quoting_style);
	  break;

	case 'c':
	  time_type = time_ctime;
	  break;

	case 'd':
	  immediate_dirs = 1;
	  break;

	case 'f':
	  /* Same as enabling -a -U and disabling -l -s.  */
	  all_files = 1;
	  really_all_files = 1;
	  sort_type = sort_none;
	  sort_type_specified = 1;
	  /* disable -l */
	  if (format == long_format)
	    format = (isatty (STDOUT_FILENO) ? many_per_line : one_per_line);
	  print_block_size = 0;	/* disable -s */
	  print_with_color = 0;	/* disable --color */
	  break;

	case 'g':
	  /* No effect.  For BSD compatibility. */
	  break;

	case 'h':
	  output_block_size = -1024;
	  break;

	case 'H':
	  output_block_size = -1000;
	  break;

	case 'i':
	  print_inode = 1;
	  break;

	case 'k':
	  output_block_size = 1024;
	  break;

	case 'l':
	  format = long_format;
	  break;

	case 'm':
	  format = with_commas;
	  break;

	case 'n':
	  numeric_ids = 1;
	  break;

	case 'o':  /* Just like -l, but don't display group info.  */
	  format = long_format;
	  inhibit_group = 1;
	  break;

	case 'p':
	  indicator_style = file_type;
	  break;

	case 'q':
	  qmark_funny_chars = 1;
	  break;

	case 'r':
	  sort_reverse = 1;
	  break;

	case 's':
	  print_block_size = 1;
	  break;

	case 't':
	  sort_type = sort_time;
	  sort_type_specified = 1;
	  break;

	case 'u':
	  time_type = time_atime;
	  break;

	case 'v':
	  sort_type = sort_version;
	  sort_type_specified = 1;
	  break;

	case 'w':
	  if (xstrtol (optarg, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long <= 0 || tmp_long > INT_MAX)
	    error (EXIT_FAILURE, 0, _("invalid line width: %s"),
		   quotearg (optarg));
	  line_length = (int) tmp_long;
	  break;

	case 'x':
	  format = horizontal;
	  break;

	case 'A':
	  really_all_files = 0;
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
	  indicator_style = classify;
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
	  set_quoting_style (NULL, literal_quoting_style);
	  break;

	case 'Q':
	  set_quoting_style (NULL, c_quoting_style);
	  break;

	case 'R':
	  trace_dirs = 1;
	  break;

	case 'S':
	  sort_type = sort_size;
	  sort_type_specified = 1;
	  break;

	case 'T':
	  if (xstrtol (optarg, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long < 0 || tmp_long > INT_MAX)
	    error (EXIT_FAILURE, 0, _("invalid tab size: %s"),
		   quotearg (optarg));
	  tabsize = (int) tmp_long;
	  break;

	case 'U':
	  sort_type = sort_none;
	  sort_type_specified = 1;
	  break;

	case 'X':
	  sort_type = sort_extension;
	  sort_type_specified = 1;
	  break;

	case '1':
	  format = one_per_line;
	  break;

	case SORT_OPTION:
	  sort_type = XARGMATCH ("--sort", optarg, sort_args, sort_types);
	  sort_type_specified = 1;
	  break;

	case TIME_OPTION:
	  time_type = XARGMATCH ("--time", optarg, time_args, time_types);
	  break;

	case FORMAT_OPTION:
	  format = XARGMATCH ("--format", optarg, format_args, format_types);
	  break;

	case COLOR_OPTION:
	  if (optarg)
	    i = XARGMATCH ("--color", optarg, color_args, color_types);
	  else
	    /* Using --color with no argument is equivalent to using
	       --color=always.  */
	    i = color_always;

	  print_with_color = (i == color_always
			      || (i == color_if_tty
				  && isatty (STDOUT_FILENO)));

	  if (print_with_color)
	    {
	      /* Don't use TAB characters in output.  Some terminal
		 emulators can't handle the combination of tabs and
		 color codes on the same line.  */
	      tabsize = 0;
	    }
	  break;

	case INDICATOR_STYLE_OPTION:
	  indicator_style = XARGMATCH ("--indicator-style", optarg,
				       indicator_style_args,
				       indicator_style_types);
	  break;

	case QUOTING_STYLE_OPTION:
	  set_quoting_style (NULL,
			     XARGMATCH ("--quoting-style", optarg,
					quoting_style_args,
					quoting_style_vals));
	  break;

	case SHOW_CONTROL_CHARS_OPTION:
	  qmark_funny_chars = 0;
	  break;

	case BLOCK_SIZE_OPTION:
	  human_block_size (optarg, 1, &output_block_size);
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  filename_quoting_options = clone_quoting_options (NULL);
  if (get_quoting_style (filename_quoting_options) == escape_quoting_style)
    set_char_quoting (filename_quoting_options, ' ', 1);
  if (indicator_style != none)
    for (p = "*=@|" + (int) indicator_style - 1;  *p;  p++)
      set_char_quoting (filename_quoting_options, *p, 1);

  dirname_quoting_options = clone_quoting_options (NULL);
  set_char_quoting (dirname_quoting_options, ':', 1);

  /* If -c or -u is specified and not -l (or any other option that implies -l),
     and no sort-type was specified, then sort by the ctime (-c) or atime (-u).
     The behavior of ls when using either -c or -u but with neither -l nor -t
     appears to be unspecified by POSIX.  So, with GNU ls, `-u' alone means
     sort by atime (this is the one that's not specified by the POSIX spec),
     -lu means show atime and sort by name, -lut means show atime and sort
     by atime.  */

  if ((time_type == time_ctime || time_type == time_atime)
      && !sort_type_specified && format != long_format)
    {
      sort_type = sort_time;
    }

  return optind;
}

/* Parse a string as part of the LS_COLORS variable; this may involve
   decoding all kinds of escape characters.  If equals_end is set an
   unescaped equal sign ends the string, otherwise only a : or \0
   does.  Returns the number of characters output, or -1 on failure.

   The resulting string is *not* null-terminated, but may contain
   embedded nulls.

   Note that both dest and src are char **; on return they point to
   the first free byte after the array and the character that ended
   the input string, respectively.  */

static int
get_funky_string (char **dest, const char **src, int equals_end)
{
  int num;			/* For numerical codes */
  int count;			/* Something to count with */
  enum {
    ST_GND, ST_BACKSLASH, ST_OCTAL, ST_HEX, ST_CARET, ST_END, ST_ERROR
  } state;
  const char *p;
  char *q;

  p = *src;			/* We don't want to double-indirect */
  q = *dest;			/* the whole darn time.  */

  count = 0;			/* No characters counted in yet.  */
  num = 0;

  state = ST_GND;		/* Start in ground state.  */
  while (state < ST_END)
    {
      switch (state)
	{
	case ST_GND:		/* Ground state (no escapes) */
	  switch (*p)
	    {
	    case ':':
	    case '\0':
	      state = ST_END;	/* End of string */
	      break;
	    case '\\':
	      state = ST_BACKSLASH; /* Backslash scape sequence */
	      ++p;
	      break;
	    case '^':
	      state = ST_CARET; /* Caret escape */
	      ++p;
	      break;
	    case '=':
	      if (equals_end)
		{
		  state = ST_END; /* End */
		  break;
		}
	      /* else fall through */
	    default:
	      *(q++) = *(p++);
	      ++count;
	      break;
	    }
	  break;

	case ST_BACKSLASH:	/* Backslash escaped character */
	  switch (*p)
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	      state = ST_OCTAL;	/* Octal sequence */
	      num = *p - '0';
	      break;
	    case 'x':
	    case 'X':
	      state = ST_HEX;	/* Hex sequence */
	      num = 0;
	      break;
	    case 'a':		/* Bell */
	      num = 7;		/* Not all C compilers know what \a means */
	      break;
	    case 'b':		/* Backspace */
	      num = '\b';
	      break;
	    case 'e':		/* Escape */
	      num = 27;
	      break;
	    case 'f':		/* Form feed */
	      num = '\f';
	      break;
	    case 'n':		/* Newline */
	      num = '\n';
	      break;
	    case 'r':		/* Carriage return */
	      num = '\r';
	      break;
	    case 't':		/* Tab */
	      num = '\t';
	      break;
	    case 'v':		/* Vtab */
	      num = '\v';
	      break;
	    case '?':		/* Delete */
              num = 127;
	      break;
	    case '_':		/* Space */
	      num = ' ';
	      break;
	    case '\0':		/* End of string */
	      state = ST_ERROR;	/* Error! */
	      break;
	    default:		/* Escaped character like \ ^ : = */
	      num = *p;
	      break;
	    }
	  if (state == ST_BACKSLASH)
	    {
	      *(q++) = num;
	      ++count;
	      state = ST_GND;
	    }
	  ++p;
	  break;

	case ST_OCTAL:		/* Octal sequence */
	  if (*p < '0' || *p > '7')
	    {
	      *(q++) = num;
	      ++count;
	      state = ST_GND;
	    }
	  else
	    num = (num << 3) + (*(p++) - '0');
	  break;

	case ST_HEX:		/* Hex sequence */
	  switch (*p)
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	      num = (num << 4) + (*(p++) - '0');
	      break;
	    case 'a':
	    case 'b':
	    case 'c':
	    case 'd':
	    case 'e':
	    case 'f':
	      num = (num << 4) + (*(p++) - 'a') + 10;
	      break;
	    case 'A':
	    case 'B':
	    case 'C':
	    case 'D':
	    case 'E':
	    case 'F':
	      num = (num << 4) + (*(p++) - 'A') + 10;
	      break;
	    default:
	      *(q++) = num;
	      ++count;
	      state = ST_GND;
	      break;
	    }
	  break;

	case ST_CARET:		/* Caret escape */
	  state = ST_GND;	/* Should be the next state... */
	  if (*p >= '@' && *p <= '~')
	    {
	      *(q++) = *(p++) & 037;
	      ++count;
	    }
	  else if (*p == '?')
	    {
	      *(q++) = 127;
	      ++count;
	    }
	  else
	    state = ST_ERROR;
	  break;

	default:
	  abort ();
	}
    }

  *dest = q;
  *src = p;

  return state == ST_ERROR ? -1 : count;
}

static void
parse_ls_color (void)
{
  const char *p;		/* Pointer to character being parsed */
  char *buf;			/* color_buf buffer pointer */
  int state;			/* State of parser */
  int ind_no;			/* Indicator number */
  char label[3];		/* Indicator label */
  struct color_ext_type *ext;	/* Extension we are working on */

  if ((p = getenv ("LS_COLORS")) == NULL || *p == '\0')
    return;

  ext = NULL;
  strcpy (label, "??");

  /* This is an overly conservative estimate, but any possible
     LS_COLORS string will *not* generate a color_buf longer than
     itself, so it is a safe way of allocating a buffer in
     advance.  */
  buf = color_buf = xstrdup (p);

  state = 1;
  while (state > 0)
    {
      switch (state)
	{
	case 1:		/* First label character */
	  switch (*p)
	    {
	    case ':':
	      ++p;
	      break;

	    case '*':
	      /* Allocate new extension block and add to head of
		 linked list (this way a later definition will
		 override an earlier one, which can be useful for
		 having terminal-specific defs override global).  */

	      ext = (struct color_ext_type *)
		xmalloc (sizeof (struct color_ext_type));
	      ext->next = color_ext_list;
	      color_ext_list = ext;

	      ++p;
	      ext->ext.string = buf;

	      state = (ext->ext.len =
		       get_funky_string (&buf, &p, 1)) < 0 ? -1 : 4;
	      break;

	    case '\0':
	      state = 0;	/* Done! */
	      break;

	    default:	/* Assume it is file type label */
	      label[0] = *(p++);
	      state = 2;
	      break;
	    }
	  break;

	case 2:		/* Second label character */
	  if (*p)
	    {
	      label[1] = *(p++);
	      state = 3;
	    }
	  else
	    state = -1;	/* Error */
	  break;

	case 3:		/* Equal sign after indicator label */
	  state = -1;	/* Assume failure... */
	  if (*(p++) == '=')/* It *should* be... */
	    {
	      for (ind_no = 0; indicator_name[ind_no] != NULL; ++ind_no)
		{
		  if (STREQ (label, indicator_name[ind_no]))
		    {
		      color_indicator[ind_no].string = buf;
		      state = ((color_indicator[ind_no].len =
				get_funky_string (&buf, &p, 0)) < 0 ? -1 : 1);
		      break;
		    }
		}
	      if (state == -1)
		error (0, 0, _("unrecognized prefix: %s"), quotearg (label));
	    }
	 break;

	case 4:		/* Equal sign after *.ext */
	  if (*(p++) == '=')
	    {
	      ext->seq.string = buf;
	      state = (ext->seq.len =
		       get_funky_string (&buf, &p, 0)) < 0 ? -1 : 1;
	    }
	  else
	    state = -1;
	  break;
	}
    }

  if (state < 0)
    {
      struct color_ext_type *e;
      struct color_ext_type *e2;

      error (0, 0,
	     _("unparsable value for LS_COLORS environment variable"));
      free (color_buf);
      for (e = color_ext_list; e != NULL; /* empty */)
	{
	  e2 = e;
	  e = e->next;
	  free (e2);
	}
      print_with_color = 0;
    }

  if (color_indicator[C_LINK].len == 6
      && !strncmp (color_indicator[C_LINK].string, "target", 6))
    color_symlink_as_referent = 1;
}

/* Request that the directory named `name' have its contents listed later.
   If `realname' is nonzero, it will be used instead of `name' when the
   directory name is printed.  This allows symbolic links to directories
   to be treated as regular directories but still be listed under their
   real names. */

static void
queue_directory (const char *name, const char *realname)
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
  register uintmax_t total_blocks = 0;

  errno = 0;
  reading = opendir (name);
  if (!reading)
    {
      error (0, errno, "%s", quotearg_colon (name));
      exit_status = 1;
      return;
    }

  /* Read the directory entries, and insert the subfiles into the `files'
     table.  */

  clear_files ();

  while ((next = readdir (reading)) != NULL)
    if (file_interesting (next))
      {
	enum filetype type = unknown;

#if HAVE_STRUCT_DIRENT_D_TYPE
	if (next->d_type == DT_DIR || next->d_type == DT_CHR
	    || next->d_type == DT_BLK || next->d_type == DT_SOCK
	    || next->d_type == DT_FIFO)
	  type = next->d_type;
#endif
	total_blocks += gobble_file (next->d_name, type, 0, name);
      }

  if (CLOSEDIR (reading))
    {
      error (0, errno, "%s", quotearg_colon (name));
      exit_status = 1;
      /* Don't return; print whatever we got. */
    }

  /* Sort the directory contents.  */
  sort_files ();

  /* If any member files are subdirectories, perhaps they should have their
     contents listed rather than being mentioned here as files.  */

  if (trace_dirs)
    extract_dirs_from_files (name, 1);

  if (trace_dirs || print_dir_name)
    {
      DIRED_INDENT ();
      PUSH_CURRENT_DIRED_POS (&subdired_obstack);
      dired_pos += quote_name (stdout, realname ? realname : name,
			       dirname_quoting_options);
      PUSH_CURRENT_DIRED_POS (&subdired_obstack);
      DIRED_FPUTS_LITERAL (":\n", stdout);
    }

  if (format == long_format || print_block_size)
    {
      const char *p;
      char buf[LONGEST_HUMAN_READABLE + 1];

      DIRED_INDENT ();
      p = _("total");
      DIRED_FPUTS (p, stdout, strlen (p));
      DIRED_PUTCHAR (' ');
      p = human_readable_inexact (total_blocks, buf, ST_NBLOCKSIZE,
				  output_block_size, human_ceiling);
      DIRED_FPUTS (p, stdout, strlen (p));
      DIRED_PUTCHAR ('\n');
    }

  if (files_index)
    print_current_files ();

  if (pending_dirs)
    DIRED_PUTCHAR ('\n');
}

/* Add `pattern' to the list of patterns for which files that match are
   not listed.  */

static void
add_ignore_pattern (const char *pattern)
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
file_interesting (const struct dirent *next)
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

static uintmax_t
gobble_file (const char *name, enum filetype type, int explicit_arg,
	     const char *dirname)
{
  register uintmax_t blocks;
  register char *path;

  if (files_index == nfiles)
    {
      nfiles *= 2;
      files = (struct fileinfo *) xrealloc ((char *) files,
					    sizeof (*files) * nfiles);
    }

  files[files_index].linkname = 0;
  files[files_index].linkmode = 0;
  files[files_index].linkok = 0;

  if (explicit_arg || format_needs_stat
      || (format_needs_type && type == unknown))
    {
      /* `path' is the absolute pathname of this file. */
      int val;

      if (name[0] == '/' || dirname[0] == 0)
	path = (char *) name;
      else
	{
	  path = (char *) alloca (strlen (name) + strlen (dirname) + 2);
	  attach (path, dirname, name);
	}

      val = (trace_links
	     ? stat (path, &files[files_index].stat)
	     : lstat (path, &files[files_index].stat));

      if (val < 0)
	{
	  error (0, errno, "%s", quotearg_colon (path));
	  exit_status = 1;
	  return 0;
	}

#if USE_ACL
      if (format == long_format)
	files[files_index].have_acl =
	  (! S_ISLNK (files[files_index].stat.st_mode)
	   && 4 < acl (path, GETACLCNT, 0, NULL));
#endif

      if (S_ISLNK (files[files_index].stat.st_mode)
	  && (explicit_arg || format == long_format || check_symlink_color))
	{
	  char *linkpath;
	  struct stat linkstats;

	  get_link_name (path, &files[files_index]);
	  linkpath = make_link_path (path, files[files_index].linkname);

	  /* Avoid following symbolic links when possible, ie, when
	     they won't be traced and when no indicator is needed. */
	  if (linkpath
	      && ((explicit_arg && format != long_format)
		  || indicator_style != none
		  || check_symlink_color)
	      && stat (linkpath, &linkstats) == 0)
	    {
	      files[files_index].linkok = 1;

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
		{
		  /* Get the linked-to file's mode for the filetype indicator
		     in long listings.  */
		  files[files_index].linkmode = linkstats.st_mode;
		  files[files_index].linkok = 1;
		}
	    }
	  if (linkpath)
	    free (linkpath);
	}

      if (S_ISLNK (files[files_index].stat.st_mode))
	files[files_index].filetype = symbolic_link;
      else if (S_ISDIR (files[files_index].stat.st_mode))
	{
	  if (explicit_arg && !immediate_dirs)
	    files[files_index].filetype = arg_directory;
	  else
	    files[files_index].filetype = directory;
	}
      else
	files[files_index].filetype = normal;

      blocks = ST_NBLOCKS (files[files_index].stat);
      {
	char buf[LONGEST_HUMAN_READABLE + 1];
	int len = strlen (human_readable_inexact (blocks, buf, ST_NBLOCKSIZE,
						  output_block_size,
						  human_ceiling));
	if (block_size_size < len)
	  block_size_size = len < 7 ? len : 7;
      }
    }
  else
    {
      files[files_index].filetype = type;
#if HAVE_STRUCT_DIRENT_D_TYPE
      files[files_index].stat.st_mode = DTTOIF (type);
#endif
      blocks = 0;
    }

  files[files_index].name = xstrdup (name);
  files_index++;

  return blocks;
}

#if HAVE_SYMLINKS

/* Put the name of the file that `filename' is a symbolic link to
   into the `linkname' field of `f'. */

static void
get_link_name (const char *filename, struct fileinfo *f)
{
  char *linkbuf;
  register int linksize;

  linkbuf = (char *) alloca (PATH_MAX + 2);
  /* Some automounters give incorrect st_size for mount points.
     I can't think of a good workaround for it, though.  */
  linksize = readlink (filename, linkbuf, PATH_MAX + 1);
  if (linksize < 0)
    {
      error (0, errno, "%s", quotearg_colon (filename));
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
make_link_path (const char *path, const char *linkname)
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

/* Return nonzero if base_name (NAME) ends in `.' or `..'
   This is so we don't try to recurse on `././././. ...' */

static int
basename_is_dot_or_dotdot (const char *name)
{
  char *base = base_name (name);
  return DOT_OR_DOTDOT (base);
}

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

  /* Queue the directories last one first, because queueing reverses the
     order.  */
  for (i = files_index - 1; i >= 0; i--)
    if ((files[i].filetype == directory || files[i].filetype == arg_directory)
	&& (!recursive || !basename_is_dot_or_dotdot (files[i].name)))
      {
	if (files[i].name[0] == '/' || dirname[0] == 0)
	  {
	    queue_directory (files[i].name, files[i].linkname);
	  }
	else
	  {
	    char *path = path_concat (dirname, files[i].name, NULL);
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
    case sort_version:
      func = sort_reverse ? rev_cmp_version : compare_version;
      break;
    default:
      abort ();
    }

  qsort (files, files_index, sizeof (struct fileinfo), func);
}

/* Comparison routines for sorting the files. */

static int
compare_ctime (const struct fileinfo *file1, const struct fileinfo *file2)
{
  int diff = CTIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
rev_cmp_ctime (const struct fileinfo *file2, const struct fileinfo *file1)
{
  int diff = CTIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
compare_mtime (const struct fileinfo *file1, const struct fileinfo *file2)
{
  int diff = MTIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
rev_cmp_mtime (const struct fileinfo *file2, const struct fileinfo *file1)
{
  int diff = MTIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
compare_atime (const struct fileinfo *file1, const struct fileinfo *file2)
{
  int diff = ATIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
rev_cmp_atime (const struct fileinfo *file2, const struct fileinfo *file1)
{
  int diff = ATIME_CMP (file2->stat, file1->stat);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
compare_size (const struct fileinfo *file1, const struct fileinfo *file2)
{
  int diff = longdiff (file2->stat.st_size, file1->stat.st_size);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
rev_cmp_size (const struct fileinfo *file2, const struct fileinfo *file1)
{
  int diff = longdiff (file2->stat.st_size, file1->stat.st_size);
  if (diff == 0)
    diff = strcoll (file1->name, file2->name);
  return diff;
}

static int
compare_version (const struct fileinfo *file1, const struct fileinfo *file2)
{
  return strverscmp (file1->name, file2->name);
}

static int
rev_cmp_version (const struct fileinfo *file2, const struct fileinfo *file1)
{
  return strverscmp (file1->name, file2->name);
}

static int
compare_name (const struct fileinfo *file1, const struct fileinfo *file2)
{
  return strcoll (file1->name, file2->name);
}

static int
rev_cmp_name (const struct fileinfo *file2, const struct fileinfo *file1)
{
  return strcoll (file1->name, file2->name);
}

/* Compare file extensions.  Files with no extension are `smallest'.
   If extensions are the same, compare by filenames instead. */

static int
compare_extension (const struct fileinfo *file1, const struct fileinfo *file2)
{
  register char *base1, *base2;
  register int cmp;

  base1 = strrchr (file1->name, '.');
  base2 = strrchr (file2->name, '.');
  if (base1 == 0 && base2 == 0)
    return strcoll (file1->name, file2->name);
  if (base1 == 0)
    return -1;
  if (base2 == 0)
    return 1;
  cmp = strcoll (base1, base2);
  if (cmp == 0)
    return strcoll (file1->name, file2->name);
  return cmp;
}

static int
rev_cmp_extension (const struct fileinfo *file2, const struct fileinfo *file1)
{
  register char *base1, *base2;
  register int cmp;

  base1 = strrchr (file1->name, '.');
  base2 = strrchr (file2->name, '.');
  if (base1 == 0 && base2 == 0)
    return strcoll (file1->name, file2->name);
  if (base1 == 0)
    return -1;
  if (base2 == 0)
    return 1;
  cmp = strcoll (base1, base2);
  if (cmp == 0)
    return strcoll (file1->name, file2->name);
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
      init_column_info ();
      print_many_per_line ();
      break;

    case horizontal:
      init_column_info ();
      print_horizontal ();
      break;

    case with_commas:
      print_with_commas ();
      break;

    case long_format:
      for (i = 0; i < files_index; i++)
	{
	  print_long_format (files + i);
	  DIRED_PUTCHAR ('\n');
	}
      break;
    }
}

static void
print_long_format (const struct fileinfo *f)
{
  char modebuf[12];

  /* 7 fields that may require LONGEST_HUMAN_READABLE bytes,
     1 10-byte mode string,
     1 24-byte time string (may be longer in some locales -- see below)
       or LONGEST_HUMAN_READABLE integer,
     9 spaces, one following each of these fields, and
     1 trailing NUL byte.  */
  char init_bigbuf[7 * LONGEST_HUMAN_READABLE + 10
		   + (LONGEST_HUMAN_READABLE < 24 ? 24 : LONGEST_HUMAN_READABLE)
		   + 9 + 1];
  char *buf = init_bigbuf;
  size_t bufsize = sizeof (init_bigbuf);
  size_t s;
  char *p;
  time_t when;
  struct tm *when_local;
  const char *fmt;
  char *user_name;

#if HAVE_ST_DM_MODE
  /* Cray DMF: look at the file's migrated, not real, status */
  mode_string (f->stat.st_dm_mode, modebuf);
#else
  mode_string (f->stat.st_mode, modebuf);
#endif

  modebuf[10] = (FILE_HAS_ACL (f) ? '+' : ' ');
  modebuf[11] = '\0';

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

  if (full_time)
    {
      fmt = _("%a %b %d %H:%M:%S %Y");
    }
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
	  fmt = _("%b %e  %Y");
	}
      else
	{
	  fmt = _("%b %e %H:%M");
	}
    }

  p = buf;

  if (print_inode)
    {
      char hbuf[LONGEST_HUMAN_READABLE + 1];
      sprintf (p, "%*s ", INODE_DIGITS,
	       human_readable ((uintmax_t) f->stat.st_ino, hbuf, 1, 1));
      p += strlen (p);
    }

  if (print_block_size)
    {
      char hbuf[LONGEST_HUMAN_READABLE + 1];
      sprintf (p, "%*s ", block_size_size,
	       human_readable_inexact ((uintmax_t) ST_NBLOCKS (f->stat), hbuf,
				       ST_NBLOCKSIZE, output_block_size,
				       human_ceiling));
      p += strlen (p);
    }

  /* The last byte of the mode string is the POSIX
     "optional alternate access method flag".  */
  sprintf (p, "%s %3u ", modebuf, (unsigned int) f->stat.st_nlink);
  p += strlen (p);

  user_name = (numeric_ids ? NULL : getuser (f->stat.st_uid));
  if (user_name)
    sprintf (p, "%-8.8s ", user_name);
  else
    sprintf (p, "%-8u ", (unsigned int) f->stat.st_uid);
  p += strlen (p);

  if (!inhibit_group)
    {
      char *group_name = (numeric_ids ? NULL : getgroup (f->stat.st_gid));
      if (group_name)
	sprintf (p, "%-8.8s ", group_name);
      else
	sprintf (p, "%-8u ", (unsigned int) f->stat.st_gid);
      p += strlen (p);
    }

  if (S_ISCHR (f->stat.st_mode) || S_ISBLK (f->stat.st_mode))
    sprintf (p, "%3u, %3u ", (unsigned) major (f->stat.st_rdev),
	     (unsigned) minor (f->stat.st_rdev));
  else
    {
      char hbuf[LONGEST_HUMAN_READABLE + 1];
      sprintf (p, "%8s ",
	       human_readable ((uintmax_t) f->stat.st_size, hbuf, 1,
			       output_block_size < 0 ? output_block_size : 1));
    }

  p += strlen (p);

  if ((when_local = localtime (&when)))
    {
      *p = '\1';

      while (! (s = strftime (p, buf + bufsize - p - 1, fmt, when_local))
	     && *p)
	{
	  char *newbuf = (char *) alloca (bufsize *= 2);
	  memcpy (newbuf, buf, p - buf);
	  p = newbuf + (p - buf);
	  buf = newbuf;
	}

      p += s;
      *p++ = ' ';

      /* NUL-terminate the string -- fputs (via DIRED_FPUTS) requires it.  */
      *p = '\0';
    }
  else
    {
      /* The time cannot be represented as a local time;
	 print it as a huge integer number of seconds.  */
      char hbuf[LONGEST_HUMAN_READABLE + 1];
      int width = full_time ? 24 : 12;

      if (when < 0)
	{
	  const char *num = human_readable (- (uintmax_t) when, hbuf, 1, 1);
	  int sign_width = width - strlen (num);
	  sprintf (p, "%*s%s ", sign_width < 0 ? 0 : sign_width, "-", num);
	}
      else
	sprintf (p, "%*s ", width,
		 human_readable ((uintmax_t) when, hbuf, 1, 1));

      p += strlen (p);
    }

  DIRED_INDENT ();
  DIRED_FPUTS (buf, stdout, p - buf);
  print_name_with_quoting (f->name, FILE_OR_LINK_MODE (f), f->linkok,
			   &dired_obstack);

  if (f->filetype == symbolic_link)
    {
      if (f->linkname)
	{
	  DIRED_FPUTS_LITERAL (" -> ", stdout);
	  print_name_with_quoting (f->linkname, f->linkmode, f->linkok - 1,
				   NULL);
	  if (indicator_style != none)
	    print_type_indicator (f->linkmode);
	}
    }
  else if (indicator_style != none)
    print_type_indicator (f->stat.st_mode);
}

/* Output to OUT a quoted representation of the file name NAME,
   using OPTIONS to control quoting.  Produce no output if OUT is NULL.
   Return the number of screen columns occupied by NAME's quoted
   representation.  */

static size_t
quote_name (FILE *out, const char *name, struct quoting_options const *options)
{
  char smallbuf[BUFSIZ];
  size_t len = quotearg_buffer (smallbuf, sizeof smallbuf, name, -1, options);
  char *buf;
  int displayed_width;

  if (len < sizeof smallbuf)
    buf = smallbuf;
  else
    {
      buf = (char *) alloca (len + 1);
      quotearg_buffer (buf, len + 1, name, -1, options);
    }

  if (qmark_funny_chars)
    {
#if HAVE_MBRTOWC && (MB_LEN_MAX > 1)
      if (MB_CUR_MAX > 1)
	{
	  const char *p = buf;
	  char *plimit = buf + len;
	  char *q = buf;
	  displayed_width = 0;

	  while (p < plimit)
	    switch (*p)
	      {
		case ' ': case '!': case '"': case '#': case '%':
		case '&': case '\'': case '(': case ')': case '*':
		case '+': case ',': case '-': case '.': case '/':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case ':': case ';': case '<': case '=': case '>':
		case '?':
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z':
		case '[': case '\\': case ']': case '^': case '_':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y':
		case 'z': case '{': case '|': case '}': case '~':
		  /* These characters are printable ASCII characters.  */
		  *q++ = *p++;
		  displayed_width += 1;
		  break;
		default:
		  /* If we have a multibyte sequence, copy it until we
		     reach its end, replacing each non-printable multibyte
		     character with a single question mark.  */
		  {
		    mbstate_t mbstate;
		    memset (&mbstate, 0, sizeof mbstate);
		    do
		      {
			wchar_t wc;
			size_t bytes;
			int w;

			bytes = mbrtowc (&wc, p, plimit - p, &mbstate);

			if (bytes == (size_t) -1)
			  {
			    /* An invalid multibyte sequence was
			       encountered.  Skip one input byte, and
			       put a question mark.  */
			    p++;
			    *q++ = '?';
			    displayed_width += 1;
			    break;
			  }

			if (bytes == (size_t) -2)
			  {
			    /* An incomplete multibyte character
			       at the end.  Replace it entirely with
			       a question mark.  */
			    p = plimit;
			    *q++ = '?';
			    displayed_width += 1;
			    break;
			  }

			if (bytes == 0)
			  /* A null wide character was encountered.  */
			  bytes = 1;

			w = wcwidth (wc);
			if (w >= 0)
			  {
			    /* A printable multibyte character.
			       Keep it.  */
			    for (; bytes > 0; --bytes)
			      *q++ = *p++;
			    displayed_width += w;
			  }
			else
			  {
			    /* An unprintable multibyte character.
			       Replace it entirely with a question
			       mark.  */
			    p += bytes;
			    *q++ = '?';
			    displayed_width += 1;
			  }
		      }
		    while (! mbsinit (&mbstate));
		  }
		  break;
	      }

	  /* The buffer may have shrunk.  */
	  len = q - buf;
	}
      else
#endif
	{
	  char *p = buf;
	  char *plimit = buf + len;

	  while (p < plimit)
	    {
	      if (! ISPRINT ((unsigned char) *p))
		*p = '?';
	      p++;
	    }
	  displayed_width = len;
	}
    }
  else
    {
      /* Assume unprintable characters have a displayed_width of 1.  */
#if HAVE_MBRTOWC && (MB_LEN_MAX > 1)
      if (MB_CUR_MAX > 1)
	displayed_width = mbsnwidth (buf, len,
				     (MBSW_ACCEPT_INVALID
				      | MBSW_ACCEPT_UNPRINTABLE));
      else
#endif
	displayed_width = len;
    }

  if (out != NULL)
    fwrite (buf, 1, len, out);
  return displayed_width;
}

static void
print_name_with_quoting (const char *p, unsigned int mode, int linkok,
			 struct obstack *stack)
{
  if (print_with_color)
    print_color_indicator (p, mode, linkok);

  if (stack)
    PUSH_CURRENT_DIRED_POS (stack);

  dired_pos += quote_name (stdout, p, filename_quoting_options);

  if (stack)
    PUSH_CURRENT_DIRED_POS (stack);

  if (print_with_color)
    prep_non_filename_text ();
}

static void
prep_non_filename_text (void)
{
  if (color_indicator[C_END].string != NULL)
    put_indicator (&color_indicator[C_END]);
  else
    {
      put_indicator (&color_indicator[C_LEFT]);
      put_indicator (&color_indicator[C_NORM]);
      put_indicator (&color_indicator[C_RIGHT]);
    }
}

/* Print the file name of `f' with appropriate quoting.
   Also print file size, inode number, and filetype indicator character,
   as requested by switches.  */

static void
print_file_name_and_frills (const struct fileinfo *f)
{
  char buf[LONGEST_HUMAN_READABLE + 1];

  if (print_inode)
    printf ("%*s ", INODE_DIGITS,
	    human_readable ((uintmax_t) f->stat.st_ino, buf, 1, 1));

  if (print_block_size)
    printf ("%*s ", block_size_size,
	    human_readable_inexact ((uintmax_t) ST_NBLOCKS (f->stat), buf,
				    ST_NBLOCKSIZE, output_block_size,
				    human_ceiling));

  print_name_with_quoting (f->name, FILE_OR_LINK_MODE (f), f->linkok, NULL);

  if (indicator_style != none)
    print_type_indicator (f->stat.st_mode);
}

static void
print_type_indicator (unsigned int mode)
{
  int c;

  if (S_ISREG (mode))
    {
      if (indicator_style == classify && (mode & S_IXUGO))
	c ='*';
      else
	c = 0;
    }
  else
    {
      if (S_ISDIR (mode))
	c = '/';
      else if (S_ISLNK (mode))
	c = '@';
      else if (S_ISFIFO (mode))
	c = '|';
      else if (S_ISSOCK (mode))
	c = '=';
      else if (S_ISDOOR (mode))
	c = '>';
      else
	c = 0;
    }

  if (c)
    DIRED_PUTCHAR (c);
}

static void
print_color_indicator (const char *name, unsigned int mode, int linkok)
{
  int type = C_FILE;
  struct color_ext_type *ext;	/* Color extension */
  size_t len;			/* Length of name */

  /* Is this a nonexistent file?  If so, linkok == -1.  */

  if (linkok == -1 && color_indicator[C_MISSING].string != NULL)
    {
      ext = NULL;
      type = C_MISSING;
    }
  else
    {
      if (S_ISDIR (mode))
	type = C_DIR;
      else if (S_ISLNK (mode))
	type = ((!linkok && color_indicator[C_ORPHAN].string)
		? C_ORPHAN : C_LINK);
      else if (S_ISFIFO (mode))
	type = C_FIFO;
      else if (S_ISSOCK (mode))
	type = C_SOCK;
      else if (S_ISBLK (mode))
	type = C_BLK;
      else if (S_ISCHR (mode))
	type = C_CHR;
      else if (S_ISDOOR (mode))
	type = C_DOOR;

      if (type == C_FILE && (mode & S_IXUGO) != 0)
	type = C_EXEC;

      /* Check the file's suffix only if still classified as C_FILE.  */
      ext = NULL;
      if (type == C_FILE)
	{
	  /* Test if NAME has a recognized suffix.  */

	  len = strlen (name);
	  name += len;		/* Pointer to final \0.  */
	  for (ext = color_ext_list; ext != NULL; ext = ext->next)
	    {
	      if ((size_t) ext->ext.len <= len
		  && strncmp (name - ext->ext.len, ext->ext.string,
			      ext->ext.len) == 0)
		break;
	    }
	}
    }

  put_indicator (&color_indicator[C_LEFT]);
  put_indicator (ext ? &(ext->seq) : &color_indicator[type]);
  put_indicator (&color_indicator[C_RIGHT]);
}

/* Output a color indicator (which may contain nulls).  */
static void
put_indicator (const struct bin_str *ind)
{
  register int i;
  register const char *p;

  p = ind->string;

  for (i = ind->len; i > 0; --i)
    putchar (*(p++));
}

static int
length_of_file_name_and_frills (const struct fileinfo *f)
{
  register int len = 0;

  if (print_inode)
    len += INODE_DIGITS + 1;

  if (print_block_size)
    len += 1 + block_size_size;

  len += quote_name (NULL, f->name, filename_quoting_options);

  if (indicator_style != none)
    {
      unsigned filetype = f->stat.st_mode;

      if (S_ISREG (filetype))
	{
	  if (indicator_style == classify
	      && (f->stat.st_mode & S_IXUGO))
	    len += 1;
	}
      else if (S_ISDIR (filetype)
	       || S_ISLNK (filetype)
	       || S_ISFIFO (filetype)
	       || S_ISSOCK (filetype)
	       || S_ISDOOR (filetype)
	       )
	len += 1;
    }

  return len;
}

static void
print_many_per_line (void)
{
  struct column_info *line_fmt;
  int filesno;			/* Index into files. */
  int row;			/* Current row. */
  int max_name_length;		/* Length of longest file name + frills. */
  int name_length;		/* Length of each file name + frills. */
  int pos;			/* Current character column. */
  int cols;			/* Number of files across. */
  int rows;			/* Maximum number of files down. */
  int max_cols;

  /* Normally the maximum number of columns is determined by the
     screen width.  But if few files are available this might limit it
     as well.  */
  max_cols = max_idx > files_index ? files_index : max_idx;

  /* Compute the maximum number of possible columns.  */
  for (filesno = 0; filesno < files_index; ++filesno)
    {
      int i;

      name_length = length_of_file_name_and_frills (files + filesno);

      for (i = 0; i < max_cols; ++i)
	{
	  if (column_info[i].valid_len)
	    {
	      int idx = filesno / ((files_index + i) / (i + 1));
	      int real_length = name_length + (idx == i ? 0 : 2);

	      if (real_length > column_info[i].col_arr[idx])
		{
		  column_info[i].line_len += (real_length
					   - column_info[i].col_arr[idx]);
		  column_info[i].col_arr[idx] = real_length;
		  column_info[i].valid_len = column_info[i].line_len < line_length;
		}
	    }
	}
    }

  /* Find maximum allowed columns.  */
  for (cols = max_cols; cols > 1; --cols)
    {
      if (column_info[cols - 1].valid_len)
	break;
    }

  line_fmt = &column_info[cols - 1];

  /* Calculate the number of rows that will be in each column except possibly
     for a short column on the right. */
  rows = files_index / cols + (files_index % cols != 0);

  for (row = 0; row < rows; row++)
    {
      int col = 0;
      filesno = row;
      pos = 0;
      /* Print the next row.  */
      while (1)
	{
	  print_file_name_and_frills (files + filesno);
	  name_length = length_of_file_name_and_frills (files + filesno);
	  max_name_length = line_fmt->col_arr[col++];

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
  struct column_info *line_fmt;
  int filesno;
  int max_name_length;
  int name_length;
  int cols;
  int pos;
  int max_cols;

  /* Normally the maximum number of columns is determined by the
     screen width.  But if few files are available this might limit it
     as well.  */
  max_cols = max_idx > files_index ? files_index : max_idx;

  /* Compute the maximum file name length.  */
  max_name_length = 0;
  for (filesno = 0; filesno < files_index; ++filesno)
    {
      int i;

      name_length = length_of_file_name_and_frills (files + filesno);

      for (i = 0; i < max_cols; ++i)
	{
	  if (column_info[i].valid_len)
	    {
	      int idx = filesno % (i + 1);
	      int real_length = name_length + (idx == i ? 0 : 2);

	      if (real_length > column_info[i].col_arr[idx])
		{
		  column_info[i].line_len += (real_length
					   - column_info[i].col_arr[idx]);
		  column_info[i].col_arr[idx] = real_length;
		  column_info[i].valid_len = column_info[i].line_len < line_length;
		}
	    }
	}
    }

  /* Find maximum allowed columns.  */
  for (cols = max_cols; cols > 1; --cols)
    {
      if (column_info[cols - 1].valid_len)
	break;
    }

  line_fmt = &column_info[cols - 1];

  pos = 0;

  /* Print first entry.  */
  print_file_name_and_frills (files);
  name_length = length_of_file_name_and_frills (files);
  max_name_length = line_fmt->col_arr[0];

  /* Now the rest.  */
  for (filesno = 1; filesno < files_index; ++filesno)
    {
      int col = filesno % cols;

      if (col == 0)
	{
	  putchar ('\n');
	  pos = 0;
	}
      else
	{
	  indent (pos + name_length, pos + max_name_length);
	  pos += max_name_length;
	}

      print_file_name_and_frills (files + filesno);

      name_length = length_of_file_name_and_frills (files + filesno);
      max_name_length = line_fmt->col_arr[col];
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
      if (tabsize > 0 && to / tabsize > (from + 1) / tabsize)
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
/* FIXME: maybe remove this function someday.  See about using a
   non-malloc'ing version of path_concat.  */

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
init_column_info (void)
{
  int i;
  int allocate = 0;

  max_idx = line_length / MIN_COLUMN_WIDTH;
  if (max_idx == 0)
    max_idx = 1;

  if (column_info == NULL)
    {
      column_info = (struct column_info *) xmalloc (max_idx
						    * sizeof (struct column_info));
      allocate = 1;
    }

  for (i = 0; i < max_idx; ++i)
    {
      int j;

      column_info[i].valid_len = 1;
      column_info[i].line_len = (i + 1) * MIN_COLUMN_WIDTH;

      if (allocate)
	column_info[i].col_arr = (int *) xmalloc ((i + 1) * sizeof (int));

      for (j = 0; j <= i; ++j)
	column_info[i].col_arr[j] = MIN_COLUMN_WIDTH;
    }
}

void
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
  -a, --all                  do not hide entries starting with .\n\
  -A, --almost-all           do not list implied . and ..\n\
  -b, --escape               print octal escapes for nongraphic characters\n\
      --block-size=SIZE      use SIZE-byte blocks\n\
  -B, --ignore-backups       do not list implied entries ending with ~\n\
  -c                         with -lt: sort by, and show, ctime (time of last\n\
				 modification of file status information)\n\
                               with -l: show ctime and sort by name\n\
                               otherwise: sort by ctime\n\
  -C                         list entries by columns\n\
      --color[=WHEN]         control whether color is used to distinguish file\n\
                               types.  WHEN may be `never', `always', or `auto'\n\
  -d, --directory            list directory entries instead of contents\n\
  -D, --dired                generate output designed for Emacs' dired mode\n\
  -f                         do not sort, enable -aU, disable -lst\n\
  -F, --classify             append indicator (one of */=@|) to entries\n\
      --format=WORD          across -x, commas -m, horizontal -x, long -l,\n\
                               single-column -1, verbose -l, vertical -C\n\
      --full-time            list both full date and full time\n"));

      printf (_("\
  -g                         (ignored)\n\
  -G, --no-group             inhibit display of group information\n\
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
  -H, --si                   likewise, but use powers of 1000 not 1024\n\
      --indicator-style=WORD append indicator with style WORD to entry names:\n\
                               none (default), classify (-F), file-type (-p)\n\
  -i, --inode                print index number of each file\n\
  -I, --ignore=PATTERN       do not list implied entries matching shell PATTERN\n\
  -k, --kilobytes            like --block-size=1024\n\
  -l                         use a long listing format\n\
  -L, --dereference          list entries pointed to by symbolic links\n\
  -m                         fill width with a comma separated list of entries\n\
  -n, --numeric-uid-gid      list numeric UIDs and GIDs instead of names\n\
  -N, --literal              print raw entry names (don't treat e.g. control\n\
                               characters specially)\n\
  -o                         use long listing format without group info\n\
  -p, --file-type            append indicator (one of /=@|) to entries\n\
  -q, --hide-control-chars   print ? instead of non graphic characters\n\
      --show-control-chars   show non graphic characters as-is (default\n\
                             unless program is `ls' and output is a terminal)\n\
  -Q, --quote-name           enclose entry names in double quotes\n\
      --quoting-style=WORD   use quoting style WORD for entry names:\n\
                               literal, locale, shell, shell-always, c, escape\n\
  -r, --reverse              reverse order while sorting\n\
  -R, --recursive            list subdirectories recursively\n\
  -s, --size                 print size of each file, in blocks\n"));

      printf (_("\
  -S                         sort by file size\n\
      --sort=WORD            extension -X, none -U, size -S, time -t,\n\
                               version -v\n\
                             status -c, time -t, atime -u, access -u, use -u\n\
      --time=WORD            show time as WORD instead of modification time:\n\
                               atime, access, use, ctime or status; use\n\
                               specified time as sort key if --sort=time\n\
  -t                         sort by modification time\n\
  -T, --tabsize=COLS         assume tab stops at each COLS instead of 8\n\
  -u                         with -lt: sort by, and show, access time\n\
                               with -l: show access time and sort by name\n\
                               otherwise: sort by access time\n\
  -U                         do not sort; list entries in directory order\n\
  -v                         sort by version\n\
  -w, --width=COLS           assume screen width instead of current value\n\
  -x                         list entries by lines instead of by columns\n\
  -X                         sort alphabetically by entry extension\n\
  -1                         list one file per line\n\
      --help                 display this help and exit\n\
      --version              output version information and exit\n\
\n\
By default, color is not used to distinguish types of files.  That is\n\
equivalent to using --color=none.  Using the --color option without the\n\
optional WHEN argument is equivalent to using --color=always.  With\n\
--color=auto, color codes are output only if standard output is connected\n\
to a terminal (tty).\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}
