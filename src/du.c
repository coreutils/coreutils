/* du -- summarize disk usage
   Copyright (C) 1988-1991, 1995-2005 Free Software Foundation, Inc.

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

/* Differences from the Unix du:
   * Doesn't simply ignore the names of regular files given as arguments
     when -a is given.

   By tege@sics.se, Torbjorn Granlund,
   and djm@ai.mit.edu, David MacKenzie.
   Variable blocks added by lm@sgi.com and eggert@twinsun.com.
   Rewritten to use nftw, then to use fts by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>
#include "system.h"
#include "argmatch.h"
#include "dirname.h" /* for strip_trailing_slashes */
#include "error.h"
#include "exclude.h"
#include "fprintftime.h"
#include "hash.h"
#include "human.h"
#include "inttostr.h"
#include "quote.h"
#include "quotearg.h"
#include "readtokens0.h"
#include "same.h"
#include "stat-time.h"
#include "xfts.h"
#include "xstrtol.h"

extern bool fts_debug;

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "du"

#define AUTHORS \
  "Torbjorn Granlund", "David MacKenzie, Paul Eggert", "Jim Meyering"

#if DU_DEBUG
# define FTS_CROSS_CHECK(Fts) fts_cross_check (Fts)
# define DEBUG_OPT "d"
#else
# define FTS_CROSS_CHECK(Fts)
# define DEBUG_OPT
#endif

/* Initial size of the hash table.  */
#define INITIAL_TABLE_SIZE 103

/* Hash structure for inode and device numbers.  The separate entry
   structure makes it easier to rehash "in place".  */

struct entry
{
  ino_t st_ino;
  dev_t st_dev;
};

/* A set of dev/ino pairs.  */
static Hash_table *htab;

/* Define a class for collecting directory information. */

struct duinfo
{
  /* Size of files in directory.  */
  uintmax_t size;

  /* Latest time stamp found.  If tmax.tv_sec == TYPE_MINIMUM (time_t)
     && tmax.tv_nsec < 0, no time stamp has been found.  */
  struct timespec tmax;
};

/* Initialize directory data.  */
static inline void
duinfo_init (struct duinfo *a)
{
  a->size = 0;
  a->tmax.tv_sec = TYPE_MINIMUM (time_t);
  a->tmax.tv_nsec = -1;
}

/* Set directory data.  */
static inline void
duinfo_set (struct duinfo *a, uintmax_t size, struct timespec tmax)
{
  a->size = size;
  a->tmax = tmax;
}

/* Accumulate directory data.  */
static inline void
duinfo_add (struct duinfo *a, struct duinfo const *b)
{
  a->size += b->size;
  if (timespec_cmp (a->tmax, b->tmax) < 0)
    a->tmax = b->tmax;
}

/* A structure for per-directory level information.  */
struct dulevel
{
  /* Entries in this directory.  */
  struct duinfo ent;

  /* Total for subdirectories.  */
  struct duinfo subdir;
};

/* Name under which this program was invoked.  */
char *program_name;

/* If true, display counts for all files, not just directories.  */
static bool opt_all = false;

/* If true, rather than using the disk usage of each file,
   use the apparent size (a la stat.st_size).  */
static bool apparent_size = false;

/* If true, count each hard link of files with multiple links.  */
static bool opt_count_all = false;

/* If true, output the NUL byte instead of a newline at the end of each line. */
static bool opt_nul_terminate_output = false;

/* If true, print a grand total at the end.  */
static bool print_grand_total = false;

/* If nonzero, do not add sizes of subdirectories.  */
static bool opt_separate_dirs = false;

/* Show the total for each directory (and file if --all) that is at
   most MAX_DEPTH levels down from the root of the hierarchy.  The root
   is at level 0, so `du --max-depth=0' is equivalent to `du -s'.  */
static size_t max_depth = SIZE_MAX;

/* Human-readable options for output.  */
static int human_output_opts;

/* If true, print most recently modified date, using the specified format.  */
static bool opt_time = false;

/* Type of time to display. controlled by --time.  */

enum time_type
  {
    time_mtime,			/* default */
    time_ctime,
    time_atime
  };

static enum time_type time_type = time_mtime;

/* User specified date / time style */
static char const *time_style = NULL;

/* Format used to display date / time. Controlled by --time-style */
static char const *time_format = NULL;

/* The units to use when printing sizes.  */
static uintmax_t output_block_size;

/* File name patterns to exclude.  */
static struct exclude *exclude;

/* Grand total size of all args, in bytes. Also latest modified date. */
static struct duinfo tot_dui;

#define IS_DIR_TYPE(Type)	\
  ((Type) == FTS_DP		\
   || (Type) == FTS_DNR)

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  APPARENT_SIZE_OPTION = CHAR_MAX + 1,
  EXCLUDE_OPTION,
  FILES0_FROM_OPTION,
  HUMAN_SI_OPTION,

  /* FIXME: --kilobytes is deprecated (but not -k); remove in late 2006 */
  KILOBYTES_LONG_OPTION,

  MAX_DEPTH_OPTION,

  /* FIXME: --megabytes is deprecated (but not -m); remove in late 2006 */
  MEGABYTES_LONG_OPTION,

  TIME_OPTION,
  TIME_STYLE_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"apparent-size", no_argument, NULL, APPARENT_SIZE_OPTION},
  {"block-size", required_argument, NULL, 'B'},
  {"bytes", no_argument, NULL, 'b'},
  {"count-links", no_argument, NULL, 'l'},
  {"dereference", no_argument, NULL, 'L'},
  {"dereference-args", no_argument, NULL, 'D'},
  {"exclude", required_argument, NULL, EXCLUDE_OPTION},
  {"exclude-from", required_argument, NULL, 'X'},
  {"files0-from", required_argument, NULL, FILES0_FROM_OPTION},
  {"human-readable", no_argument, NULL, 'h'},
  {"si", no_argument, NULL, HUMAN_SI_OPTION},
  {"kilobytes", no_argument, NULL, KILOBYTES_LONG_OPTION},
  {"max-depth", required_argument, NULL, MAX_DEPTH_OPTION},
  {"null", no_argument, NULL, '0'},
  {"megabytes", no_argument, NULL, MEGABYTES_LONG_OPTION},
  {"no-dereference", no_argument, NULL, 'P'},
  {"one-file-system", no_argument, NULL, 'x'},
  {"separate-dirs", no_argument, NULL, 'S'},
  {"summarize", no_argument, NULL, 's'},
  {"total", no_argument, NULL, 'c'},
  {"time", optional_argument, NULL, TIME_OPTION},
  {"time-style", required_argument, NULL, TIME_STYLE_OPTION},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

static char const *const time_args[] =
{
  "atime", "access", "use", "ctime", "status", NULL
};
static enum time_type const time_types[] =
{
  time_atime, time_atime, time_atime, time_ctime, time_ctime
};
ARGMATCH_VERIFY (time_args, time_types);

/* `full-iso' uses full ISO-style dates and times.  `long-iso' uses longer
   ISO-style time stamps, though shorter than `full-iso'.  `iso' uses shorter
   ISO-style time stamps.  */
enum time_style
  {
    full_iso_time_style,       /* --time-style=full-iso */
    long_iso_time_style,       /* --time-style=long-iso */
    iso_time_style	       /* --time-style=iso */
  };

static char const *const time_style_args[] =
{
  "full-iso", "long-iso", "iso", NULL
};
static enum time_style const time_style_types[] =
{
  full_iso_time_style, long_iso_time_style, iso_time_style
};
ARGMATCH_VERIFY (time_style_args, time_style_types);

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]...\n\
  or:  %s [OPTION]... --files0-from=F\n\
"), program_name, program_name);
      fputs (_("\
Summarize disk usage of each FILE, recursively for directories.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -a, --all             write counts for all files, not just directories\n\
      --apparent-size   print apparent sizes, rather than disk usage; although\n\
                          the apparent size is usually smaller, it may be\n\
                          larger due to holes in (`sparse') files, internal\n\
                          fragmentation, indirect blocks, and the like\n\
  -B, --block-size=SIZE use SIZE-byte blocks\n\
  -b, --bytes           equivalent to `--apparent-size --block-size=1'\n\
  -c, --total           produce a grand total\n\
  -D, --dereference-args  dereference FILEs that are symbolic links\n\
"), stdout);
      fputs (_("\
      --files0-from=F   summarize disk usage of the NUL-terminated file\n\
                          names specified in file F\n\
  -H                    like --si, but also evokes a warning; will soon\n\
                          change to be equivalent to --dereference-args (-D)\n\
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
      --si              like -h, but use powers of 1000 not 1024\n\
  -k                    like --block-size=1K\n\
  -l, --count-links     count sizes many times if hard linked\n\
  -m                    like --block-size=1M\n\
"), stdout);
      fputs (_("\
  -L, --dereference     dereference all symbolic links\n\
  -P, --no-dereference  don't follow any symbolic links (this is the default)\n\
  -0, --null            end each output line with 0 byte rather than newline\n\
  -S, --separate-dirs   do not include size of subdirectories\n\
  -s, --summarize       display only a total for each argument\n\
"), stdout);
      fputs (_("\
  -x, --one-file-system  skip directories on different file systems\n\
  -X FILE, --exclude-from=FILE  Exclude files that match any pattern in FILE.\n\
      --exclude=PATTERN Exclude files that match PATTERN.\n\
      --max-depth=N     print the total for a directory (or file, with --all)\n\
                          only if it is N or fewer levels below the command\n\
                          line argument;  --max-depth=0 is the same as\n\
                          --summarize\n\
"), stdout);
      fputs (_("\
      --time            show time of the last modification of any file in the\n\
                          directory, or any of its subdirectories\n\
      --time=WORD       show time as WORD instead of modification time:\n\
                          atime, access, use, ctime or status\n\
      --time-style=STYLE show times using style STYLE:\n\
                          full-iso, long-iso, iso, +FORMAT\n\
                          FORMAT is interpreted like `date'\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\n\
SIZE may be (or may be an integer optionally followed by) one of following:\n\
kB 1000, K 1024, MB 1000*1000, M 1024*1024, and so on for G, T, P, E, Z, Y.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

static size_t
entry_hash (void const *x, size_t table_size)
{
  struct entry const *p = x;

  /* Ignoring the device number here should be fine.  */
  /* The cast to uintmax_t prevents negative remainders
     if st_ino is negative.  */
  return (uintmax_t) p->st_ino % table_size;
}

/* Compare two dev/ino pairs.  Return true if they are the same.  */
static bool
entry_compare (void const *x, void const *y)
{
  struct entry const *a = x;
  struct entry const *b = y;
  return SAME_INODE (*a, *b) ? true : false;
}

/* Try to insert the INO/DEV pair into the global table, HTAB.
   Return true if the pair is successfully inserted,
   false if the pair is already in the table.  */
static bool
hash_ins (ino_t ino, dev_t dev)
{
  struct entry *ent;
  struct entry *ent_from_table;

  ent = xmalloc (sizeof *ent);
  ent->st_ino = ino;
  ent->st_dev = dev;

  ent_from_table = hash_insert (htab, ent);
  if (ent_from_table == NULL)
    {
      /* Insertion failed due to lack of memory.  */
      xalloc_die ();
    }

  if (ent_from_table == ent)
    {
      /* Insertion succeeded.  */
      return true;
    }

  /* That pair is already in the table, so ENT was not inserted.  Free it.  */
  free (ent);

  return false;
}

/* Initialize the hash table.  */
static void
hash_init (void)
{
  htab = hash_initialize (INITIAL_TABLE_SIZE, NULL,
			  entry_hash, entry_compare, free);
  if (htab == NULL)
    xalloc_die ();
}

/* FIXME: this code is nearly identical to code in date.c  */
/* Display the date and time in WHEN according to the format specified
   in FORMAT.  */

static void
show_date (const char *format, struct timespec when)
{
  struct tm *tm = localtime (&when.tv_sec);
  if (! tm)
    {
      char buf[INT_BUFSIZE_BOUND (intmax_t)];
      error (0, 0, _("time %s is out of range"),
	     (TYPE_SIGNED (time_t)
	      ? imaxtostr (when.tv_sec, buf)
	      : umaxtostr (when.tv_sec, buf)));
      fputs (buf, stdout);
      return;
    }

  fprintftime (stdout, format, tm, 0, when.tv_nsec);
}

/* Print N_BYTES.  Convert it to a readable value before printing.  */

static void
print_only_size (uintmax_t n_bytes)
{
  char buf[LONGEST_HUMAN_READABLE + 1];
  fputs (human_readable (n_bytes, buf, human_output_opts,
			 1, output_block_size), stdout);
}

/* Print size (and optionally time) indicated by *PDUI, followed by STRING.  */

static void
print_size (const struct duinfo *pdui, const char *string)
{
  print_only_size (pdui->size);
  if (opt_time)
    {
      putchar ('\t');
      show_date (time_format, pdui->tmax);
    }
  printf ("\t%s%c", string, opt_nul_terminate_output ? '\0' : '\n');
  fflush (stdout);
}

/* This function is called once for every file system object that fts
   encounters.  fts does a depth-first traversal.  This function knows
   that and accumulates per-directory totals based on changes in
   the depth of the current entry.  It returns true on success.  */

static bool
process_file (FTS *fts, FTSENT *ent)
{
  bool ok;
  struct duinfo dui;
  struct duinfo dui_to_print;
  size_t level;
  static size_t prev_level;
  static size_t n_alloc;
  /* First element of the structure contains:
     The sum of the st_size values of all entries in the single directory
     at the corresponding level.  Although this does include the st_size
     corresponding to each subdirectory, it does not include the size of
     any file in a subdirectory. Also corresponding last modified date.
     Second element of the structure contains:
     The sum of the sizes of all entries in the hierarchy at or below the
     directory at the specified level.  */
  static struct dulevel *dulvl;
  bool print = true;

  const char *file = ent->fts_path;
  const struct stat *sb = ent->fts_statp;
  bool skip;

  /* If necessary, set FTS_SKIP before returning.  */
  skip = excluded_file_name (exclude, ent->fts_path);
  if (skip)
    fts_set (fts, ent, FTS_SKIP);

  switch (ent->fts_info)
    {
    case FTS_NS:
      error (0, ent->fts_errno, _("cannot access %s"), quote (file));
      return false;

    case FTS_ERR:
      /* if (S_ISDIR (ent->fts_statp->st_mode) && FIXME */
      error (0, ent->fts_errno, _("%s"), quote (file));
      return false;

    case FTS_DNR:
      /* Don't return just yet, since although the directory is not readable,
	 we were able to stat it, so we do have a size.  */
      error (0, ent->fts_errno, _("cannot read directory %s"), quote (file));
      ok = false;
      break;

    default:
      ok = true;
      break;
    }

  /* If this is the first (pre-order) encounter with a directory,
     or if it's the second encounter for a skipped directory, then
     return right away.  */
  if (ent->fts_info == FTS_D || skip)
    return ok;

  /* If the file is being excluded or if it has already been counted
     via a hard link, then don't let it contribute to the sums.  */
  if (skip
      || (!opt_count_all
	  && ! S_ISDIR (sb->st_mode)
	  && 1 < sb->st_nlink
	  && ! hash_ins (sb->st_ino, sb->st_dev)))
    {
      /* Note that we must not simply return here.
	 We still have to update prev_level and maybe propagate
	 some sums up the hierarchy.  */
      duinfo_init (&dui);
      print = false;
    }
  else
    {
      duinfo_set (&dui,
		  (apparent_size
		   ? sb->st_size
		   : (uintmax_t) ST_NBLOCKS (*sb) * ST_NBLOCKSIZE),
		  (time_type == time_mtime ? get_stat_mtime (sb)
		   : time_type == time_atime ? get_stat_atime (sb)
		   : get_stat_ctime (sb)));
    }

  level = ent->fts_level;
  dui_to_print = dui;

  if (n_alloc == 0)
    {
      n_alloc = level + 10;
      dulvl = xcalloc (n_alloc, sizeof *dulvl);
    }
  else
    {
      if (level == prev_level)
	{
	  /* This is usually the most common case.  Do nothing.  */
	}
      else if (level > prev_level)
	{
	  /* Descending the hierarchy.
	     Clear the accumulators for *all* levels between prev_level
	     and the current one.  The depth may change dramatically,
	     e.g., from 1 to 10.  */
	  size_t i;

	  if (n_alloc <= level)
	    {
	      dulvl = xnrealloc (dulvl, level, 2 * sizeof *dulvl);
	      n_alloc = level * 2;
	    }

	  for (i = prev_level + 1; i <= level; i++)
	    {
	      duinfo_init (&dulvl[i].ent);
	      duinfo_init (&dulvl[i].subdir);
	    }
	}
      else /* level < prev_level */
	{
	  /* Ascending the hierarchy.
	     Process a directory only after all entries in that
	     directory have been processed.  When the depth decreases,
	     propagate sums from the children (prev_level) to the parent.
	     Here, the current level is always one smaller than the
	     previous one.  */
	  assert (level == prev_level - 1);
	  duinfo_add (&dui_to_print, &dulvl[prev_level].ent);
	  if (!opt_separate_dirs)
	    duinfo_add (&dui_to_print, &dulvl[prev_level].subdir);
	  duinfo_add (&dulvl[level].subdir, &dulvl[prev_level].ent);
	  duinfo_add (&dulvl[level].subdir, &dulvl[prev_level].subdir);
	}
    }

  prev_level = level;

  /* Let the size of a directory entry contribute to the total for the
     containing directory, unless --separate-dirs (-S) is specified.  */
  if ( ! (opt_separate_dirs && IS_DIR_TYPE (ent->fts_info)))
    duinfo_add (&dulvl[level].ent, &dui);

  /* Even if this directory is unreadable or we can't chdir into it,
     do let its size contribute to the total, ... */
  duinfo_add (&tot_dui, &dui);

  /* ... but don't print out a total for it, since without the size(s)
     of any potential entries, it could be very misleading.  */
  if (ent->fts_info == FTS_DNR)
    return ok;

  /* If we're not counting an entry, e.g., because it's a hard link
     to a file we've already counted (and --count-links), then don't
     print a line for it.  */
  if (!print)
    return ok;

  if ((IS_DIR_TYPE (ent->fts_info) && level <= max_depth)
      || ((opt_all && level <= max_depth) || level == 0))
    print_size (&dui_to_print, file);

  return ok;
}

/* Recursively print the sizes of the directories (and, if selected, files)
   named in FILES, the last entry of which is NULL.
   BIT_FLAGS controls how fts works.
   Return true if successful.  */

static bool
du_files (char **files, int bit_flags)
{
  bool ok = true;

  if (*files)
    {
      FTS *fts = xfts_open (files, bit_flags, NULL);

      while (1)
	{
	  FTSENT *ent;

	  ent = fts_read (fts);
	  if (ent == NULL)
	    {
	      if (errno != 0)
		{
		  /* FIXME: try to give a better message  */
		  error (0, errno, _("fts_read failed"));
		  ok = false;
		}
	      break;
	    }
	  FTS_CROSS_CHECK (fts);

	  ok &= process_file (fts, ent);
	}

      /* Ignore failure, since the only way it can do so is in failing to
	 return to the original directory, and since we're about to exit,
	 that doesn't matter.  */
      fts_close (fts);
    }

  if (print_grand_total)
    print_size (&tot_dui, _("total"));

  return ok;
}

int
main (int argc, char **argv)
{
  int c;
  char *cwd_only[2];
  bool max_depth_specified = false;
  char **files;
  bool ok = true;
  char *files_from = NULL;
  struct Tokens tok;

  /* Bit flags that control how fts works.  */
  int bit_flags = FTS_PHYSICAL | FTS_TIGHT_CYCLE_CHECK;

  /* If true, display only a total for each argument. */
  bool opt_summarize_only = false;

  cwd_only[0] = ".";
  cwd_only[1] = NULL;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  exclude = new_exclude ();

  human_output_opts = human_options (getenv ("DU_BLOCK_SIZE"), false,
				     &output_block_size);

  while ((c = getopt_long (argc, argv, DEBUG_OPT "0abchHklmsxB:DLPSX:",
			   long_options, NULL)) != -1)
    {
      switch (c)
	{
#if DU_DEBUG
	case 'd':
	  fts_debug = true;
	  break;
#endif

	case '0':
	  opt_nul_terminate_output = true;
	  break;

	case 'a':
	  opt_all = true;
	  break;

	case APPARENT_SIZE_OPTION:
	  apparent_size = true;
	  break;

	case 'b':
	  apparent_size = true;
	  human_output_opts = 0;
	  output_block_size = 1;
	  break;

	case 'c':
	  print_grand_total = true;
	  break;

	case 'h':
	  human_output_opts = human_autoscale | human_SI | human_base_1024;
	  output_block_size = 1;
	  break;

	case 'H':  /* FIXME: remove warning and move this "case 'H'" to
		      precede --dereference-args in late 2006.  */
	  error (0, 0, _("WARNING: use --si, not -H; the meaning of the -H\
 option will soon\nchange to be the same as that of --dereference-args (-D)"));
	  /* fall through */
	case HUMAN_SI_OPTION:
	  human_output_opts = human_autoscale | human_SI;
	  output_block_size = 1;
	  break;

	case KILOBYTES_LONG_OPTION:
	  error (0, 0,
		 _("the --kilobytes option is deprecated; use -k instead"));
	  /* fall through */
	case 'k':
	  human_output_opts = 0;
	  output_block_size = 1024;
	  break;

	case MAX_DEPTH_OPTION:		/* --max-depth=N */
	  {
	    unsigned long int tmp_ulong;
	    if (xstrtoul (optarg, NULL, 0, &tmp_ulong, NULL) == LONGINT_OK
		&& tmp_ulong <= SIZE_MAX)
	      {
		max_depth_specified = true;
		max_depth = tmp_ulong;
	      }
	    else
	      {
		error (0, 0, _("invalid maximum depth %s"),
		       quote (optarg));
		ok = false;
	      }
	  }
	  break;

	case MEGABYTES_LONG_OPTION:
	  error (0, 0,
		 _("the --megabytes option is deprecated; use -m instead"));
	  /* fall through */
	case 'm':
	  human_output_opts = 0;
	  output_block_size = 1024 * 1024;
	  break;

	case 'l':
	  opt_count_all = true;
	  break;

	case 's':
	  opt_summarize_only = true;
	  break;

	case 'x':
	  bit_flags |= FTS_XDEV;
	  break;

	case 'B':
	  human_output_opts = human_options (optarg, true, &output_block_size);
	  break;

	case 'D': /* This will eventually be 'H' (-H), too.  */
	  bit_flags = FTS_COMFOLLOW;
	  break;

	case 'L': /* --dereference */
	  bit_flags = FTS_LOGICAL;
	  break;

	case 'P': /* --no-dereference */
	  bit_flags = FTS_PHYSICAL;
	  break;

	case 'S':
	  opt_separate_dirs = true;
	  break;

	case 'X':
	  if (add_exclude_file (add_exclude, exclude, optarg,
				EXCLUDE_WILDCARDS, '\n'))
	    {
	      error (0, errno, "%s", quotearg_colon (optarg));
	      ok = false;
	    }
	  break;

	case FILES0_FROM_OPTION:
	  files_from = optarg;
	  break;

	case EXCLUDE_OPTION:
	  add_exclude (exclude, optarg, EXCLUDE_WILDCARDS);
	  break;

	case TIME_OPTION:
	  opt_time = true;
	  time_type =
	    (optarg
	     ? XARGMATCH ("--time", optarg, time_args, time_types)
	     : time_mtime);
	  break;

	case TIME_STYLE_OPTION:
	  time_style = optarg;
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  ok = false;
	}
    }

  if (!ok)
    usage (EXIT_FAILURE);

  if (opt_all & opt_summarize_only)
    {
      error (0, 0, _("cannot both summarize and show all entries"));
      usage (EXIT_FAILURE);
    }

  if (opt_summarize_only && max_depth_specified && max_depth == 0)
    {
      error (0, 0,
	     _("warning: summarizing is the same as using --max-depth=0"));
    }

  if (opt_summarize_only && max_depth_specified && max_depth != 0)
    {
      unsigned long int d = max_depth;
      error (0, 0, _("warning: summarizing conflicts with --max-depth=%lu"), d);
      usage (EXIT_FAILURE);
    }

  if (opt_summarize_only)
    max_depth = 0;

  /* Process time style if printing last times.  */
  if (opt_time)
    {
      if (! time_style)
	{
	  time_style = getenv ("TIME_STYLE");

	  /* Ignore TIMESTYLE="locale", for compatibility with ls.  */
	  if (! time_style || STREQ (time_style, "locale"))
	    time_style = "long-iso";
	  else if (*time_style == '+')
	    {
	      /* Ignore anything after a newline, for compatibility
		 with ls.  */
	      char *p = strchr (time_style, '\n');
	      if (p)
		*p = '\0';
	    }
	  else
	    {
	      /* Ignore "posix-" prefix, for compatibility with ls.  */
	      static char const posix_prefix[] = "posix-";
	      while (strncmp (time_style, posix_prefix, sizeof posix_prefix - 1)
		     == 0)
		time_style += sizeof posix_prefix - 1;
	    }
	}

      if (*time_style == '+')
	time_format = time_style + 1;
      else
        {
          switch (XARGMATCH ("time style", time_style,
                             time_style_args, time_style_types))
            {
	    case full_iso_time_style:
	      time_format = "%Y-%m-%d %H:%M:%S.%N %z";
	      break;

	    case long_iso_time_style:
	      time_format = "%Y-%m-%d %H:%M";
	      break;

	    case iso_time_style:
	      time_format = "%Y-%m-%d";
	      break;
            }
        }
    }

  if (files_from)
    {
      /* When using --files0-from=F, you may not specify any files
	 on the command-line.  */
      if (optind < argc)
	{
	  error (0, 0, _("extra operand %s"), quote (argv[optind]));
	  fprintf (stderr, "%s\n",
		   _("File operands cannot be combined with --files0-from."));
	  usage (EXIT_FAILURE);
	}

      if (! (STREQ (files_from, "-") || freopen (files_from, "r", stdin)))
	error (EXIT_FAILURE, errno, _("cannot open %s for reading"),
	       quote (files_from));

      readtokens0_init (&tok);

      if (! readtokens0 (stdin, &tok) || fclose (stdin) != 0)
	error (EXIT_FAILURE, 0, _("cannot read file names from %s"),
	       quote (files_from));

      files = tok.tok;
    }
  else
    {
      files = (optind < argc ? argv + optind : cwd_only);
    }

  /* Initialize the hash structure for inode numbers.  */
  hash_init ();

  /* Report and filter out any empty file names before invoking fts.
     This works around a glitch in fts, which fails immediately
     (without looking at the other file names) when given an empty
     file name.  */
  {
    size_t i = 0;
    size_t j;

    for (j = 0; ; j++)
      {
	if (i != j)
	  files[i] = files[j];

	if ( ! files[i])
	  break;

	if (files[i][0])
	  i++;
	else
	  {
	    if (files_from)
	      {
		/* Using the standard `filename:line-number:' prefix here is
		   not totally appropriate, since NUL is the separator, not NL,
		   but it might be better than nothing.  */
		unsigned long int file_number = j + 1;
		error (0, 0, "%s:%lu: %s", quotearg_colon (files_from),
		       file_number, _("invalid zero-length file name"));
	      }
	    else
	      error (0, 0, "%s", _("invalid zero-length file name"));
	  }
      }

    ok = (i == j);
  }

  ok &= du_files (files, bit_flags);

  /* This isn't really necessary, but it does ensure we
     exercise this function.  */
  if (files_from)
    readtokens0_free (&tok);

  hash_free (htab);

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
