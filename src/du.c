/* du -- summarize disk usage
   Copyright (C) 1988-1991, 1995-2004 Free Software Foundation, Inc.

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
#include "dirname.h" /* for strip_trailing_slashes */
#include "error.h"
#include "exclude.h"
#include "hash.h"
#include "human.h"
#include "quote.h"
#include "quotearg.h"
#include "same.h"
#include "xfts.h"
#include "xstrtol.h"

extern int fts_debug;

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

/* Name under which this program was invoked.  */
char *program_name;

/* If nonzero, display counts for all files, not just directories.  */
static int opt_all = 0;

/* If nonzero, rather than using the disk usage of each file,
   use the apparent size (a la stat.st_size).  */
static int apparent_size = 0;

/* If nonzero, count each hard link of files with multiple links.  */
static int opt_count_all = 0;

/* If true, output the NUL byte instead of a newline at the end of each line. */
bool opt_nul_terminate_output = false;

/* If nonzero, print a grand total at the end.  */
static int print_totals = 0;

/* If nonzero, do not add sizes of subdirectories.  */
static int opt_separate_dirs = 0;

/* Show the total for each directory (and file if --all) that is at
   most MAX_DEPTH levels down from the root of the hierarchy.  The root
   is at level 0, so `du --max-depth=0' is equivalent to `du -s'.  */
static int max_depth = INT_MAX;

/* Human-readable options for output.  */
static int human_output_opts;

/* The units to use when printing sizes.  */
static uintmax_t output_block_size;

/* File name patterns to exclude.  */
static struct exclude *exclude;

/* Grand total size of all args, in bytes. */
static uintmax_t tot_size = 0;

/* Nonzero indicates that du should exit with EXIT_FAILURE upon completion.  */
int G_fail;

#define IS_DIR_TYPE(Type)	\
  ((Type) == FTS_DP		\
   || (Type) == FTS_DNR)

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  APPARENT_SIZE_OPTION = CHAR_MAX + 1,
  EXCLUDE_OPTION,
  HUMAN_SI_OPTION,
  MAX_DEPTH_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
  {"apparent-size", no_argument, NULL, APPARENT_SIZE_OPTION},
  {"block-size", required_argument, 0, 'B'},
  {"bytes", no_argument, NULL, 'b'},
  {"count-links", no_argument, NULL, 'l'},
  {"dereference", no_argument, NULL, 'L'},
  {"dereference-args", no_argument, NULL, 'D'},
  {"exclude", required_argument, 0, EXCLUDE_OPTION},
  {"exclude-from", required_argument, 0, 'X'},
  {"human-readable", no_argument, NULL, 'h'},
  {"si", no_argument, 0, HUMAN_SI_OPTION},
  {"kilobytes", no_argument, NULL, 'k'}, /* long form is obsolescent */
  {"max-depth", required_argument, NULL, MAX_DEPTH_OPTION},
  {"null", no_argument, NULL, '0'},
  {"megabytes", no_argument, NULL, 'm'}, /* obsolescent */
  {"no-dereference", no_argument, NULL, 'P'},
  {"one-file-system", no_argument, NULL, 'x'},
  {"separate-dirs", no_argument, NULL, 'S'},
  {"summarize", no_argument, NULL, 's'},
  {"total", no_argument, NULL, 'c'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
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
  -H                    like --si, but also evokes a warning; will soon\n\
                        change to be equivalent to --dereference-args (-D)\n\
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
      --si              like -h, but use powers of 1000 not 1024\n\
  -k                    like --block-size=1K\n\
  -l, --count-links     count sizes many times if hard linked\n\
"), stdout);
      fputs (_("\
  -L, --dereference     dereference all symbolic links\n\
  -P, --no-dereference  don't follow any symbolic links (this is the default)\n\
  -0, --null            end each output line with 0 byte rather than newline\n\
  -S, --separate-dirs   do not include size of subdirectories\n\
  -s, --summarize       display only a total for each argument\n\
"), stdout);
      fputs (_("\
  -x, --one-file-system  skip directories on different filesystems\n\
  -X FILE, --exclude-from=FILE  Exclude files that match any pattern in FILE.\n\
      --exclude=PATTERN Exclude files that match PATTERN.\n\
      --max-depth=N     print the total for a directory (or file, with --all)\n\
                          only if it is N or fewer levels below the command\n\
                          line argument;  --max-depth=0 is the same as\n\
                          --summarize\n\
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
   If the pair is successfully inserted, return zero.
   Upon failed memory allocation exit nonzero.
   If the pair is already in the table, return nonzero.  */
static int
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
      return 0;
    }

  /* That pair is already in the table, so ENT was not inserted.  Free it.  */
  free (ent);

  return 1;
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

/* Print N_BYTES.  Convert it to a readable value before printing.  */

static void
print_only_size (uintmax_t n_bytes)
{
  char buf[LONGEST_HUMAN_READABLE + 1];
  fputs (human_readable (n_bytes, buf, human_output_opts,
			 1, output_block_size), stdout);
}

/* Print N_BYTES followed by STRING on a line.
   Convert N_BYTES to a readable value before printing.  */

static void
print_size (uintmax_t n_bytes, const char *string)
{
  print_only_size (n_bytes);
  printf ("\t%s%c", string, opt_nul_terminate_output ? '\0' : '\n');
  fflush (stdout);
}

/* This function is called once for every file system object that fts
   encounters.  fts does a depth-first traversal.  This function knows
   that and accumulates per-directory totals based on changes in
   the depth of the current entry.  */

static void
process_file (FTS *fts, FTSENT *ent)
{
  uintmax_t size;
  uintmax_t size_to_print;
  static int first_call = 1;
  static size_t prev_level;
  static size_t n_alloc;
  /* The sum of the st_size values of all entries in the single directory
     at the corresponding level.  Although this does include the st_size
     corresponding to each subdirectory, it does not include the size of
     any file in a subdirectory.  */
  static uintmax_t *sum_ent;

  /* The sum of the sizes of all entries in the hierarchy at or below the
     directory at the specified level.  */
  static uintmax_t *sum_subdir;
  int print = 1;

  const char *file = ent->fts_path;
  const struct stat *sb = ent->fts_statp;
  int skip;

  /* If necessary, set FTS_SKIP before returning.  */
  skip = excluded_filename (exclude, ent->fts_name);
  if (skip)
    fts_set (fts, ent, FTS_SKIP);

  switch (ent->fts_info)
    {
    case FTS_NS:
      error (0, ent->fts_errno, _("cannot access %s"), quote (file));
      G_fail = 1;
      return;

    case FTS_ERR:
      /* if (S_ISDIR (ent->fts_statp->st_mode) && FIXME */
      error (0, ent->fts_errno, _("%s"), quote (file));
      G_fail = 1;
      return;

    case FTS_DNR:
      /* Don't return just yet, since although the directory is not readable,
	 we were able to stat it, so we do have a size.  */
      error (0, ent->fts_errno, _("cannot read directory %s"), quote (file));
      G_fail = 1;
      break;

    default:
      break;
    }

  /* If this is the first (pre-order) encounter with a directory,
     or if it's the second encounter for a skipped directory, then
     return right away.  */
  if (ent->fts_info == FTS_D || skip)
    return;

  /* If the file is being excluded or if it has already been counted
     via a hard link, then don't let it contribute to the sums.  */
  if (skip
      || (!opt_count_all
	  && 1 < sb->st_nlink
	  && hash_ins (sb->st_ino, sb->st_dev)))
    {
      /* Note that we must not simply return here.
	 We still have to update prev_level and maybe propagate
	 some sums up the hierarchy.  */
      size = 0;
      print = 0;
    }
  else
    {
      size = (apparent_size
	      ? sb->st_size
	      : ST_NBLOCKS (*sb) * ST_NBLOCKSIZE);
    }

  if (first_call)
    {
      n_alloc = ent->fts_level + 10;
      sum_ent = XCALLOC (uintmax_t, n_alloc);
      sum_subdir = XCALLOC (uintmax_t, n_alloc);
    }
  else
    {
      /* FIXME: it's a shame that we need these `size_t' casts to avoid
	 warnings from gcc about `comparison between signed and unsigned'.
	 Probably unavoidable, assuming that the struct members
	 are of type `int' (historical), since I want variables like
	 n_alloc and prev_level to have types that make sense.  */
      if (n_alloc <= (size_t) ent->fts_level)
	{
	  n_alloc = ent->fts_level * 2;
	  sum_ent = XREALLOC (sum_ent, uintmax_t, n_alloc);
	  sum_subdir = XREALLOC (sum_subdir, uintmax_t, n_alloc);
	}
    }

  size_to_print = size;

  if (! first_call)
    {
      if ((size_t) ent->fts_level == prev_level)
	{
	  /* This is usually the most common case.  Do nothing.  */
	}
      else if (ent->fts_level > prev_level)
	{
	  /* Descending the hierarchy.
	     Clear the accumulators for *all* levels between prev_level
	     and the current one.  The depth may change dramatically,
	     e.g., from 1 to 10.  */
	  int i;
	  for (i = prev_level + 1; i <= ent->fts_level; i++)
	    {
	      sum_ent[i] = 0;
	      sum_subdir[i] = 0;
	    }
	}
      else /* ent->fts_level < prev_level */
	{
	  /* Ascending the hierarchy.
	     Process a directory only after all entries in that
	     directory have been processed.  When the depth decreases,
	     propagate sums from the children (prev_level) to the parent.
	     Here, the current level is always one smaller than the
	     previous one.  */
	  assert ((size_t) ent->fts_level == prev_level - 1);
	  size_to_print += sum_ent[prev_level];
	  if (!opt_separate_dirs)
	    size_to_print += sum_subdir[prev_level];
	  sum_subdir[ent->fts_level] += (sum_ent[prev_level]
					 + sum_subdir[prev_level]);
	}
    }

  prev_level = ent->fts_level;
  first_call = 0;

  /* Let the size of a directory entry contribute to the total for the
     containing directory, unless --separate-dirs (-S) is specified.  */
  if ( ! (opt_separate_dirs && IS_DIR_TYPE (ent->fts_info)))
    sum_ent[ent->fts_level] += size;

  /* Even if this directory is unreadable or we can't chdir into it,
     do let its size contribute to the total, ... */
  tot_size += size;

  /* ... but don't print out a total for it, since without the size(s)
     of any potential entries, it could be very misleading.  */
  if (ent->fts_info == FTS_DNR)
    return;

  /* If we're not counting an entry, e.g., because it's a hard link
     to a file we've already counted (and --count-links), then don't
     print a line for it.  */
  if (!print)
    return;

  if ((IS_DIR_TYPE (ent->fts_info) && ent->fts_level <= max_depth)
      || ((opt_all && ent->fts_level <= max_depth) || ent->fts_level == 0))
    {
      print_only_size (size_to_print);
      fputc ('\t', stdout);
      fputs (file, stdout);
      fputc (opt_nul_terminate_output ? '\0' : '\n', stdout);
      fflush (stdout);
    }
}

/* Recursively print the sizes of the directories (and, if selected, files)
   named in FILES, the last entry of which is NULL.
   BIT_FLAGS controls how fts works.
   If the fts_open call fails, exit nonzero.
   Otherwise, return nonzero upon error.  */

static int
du_files (char **files, int bit_flags)
{
  int fail = 0;

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
	      fail = 1;
	    }
	  break;
	}
      FTS_CROSS_CHECK (fts);

      /* This is a space optimization.  If we aren't printing totals,
	 then it's ok to clear the duplicate-detection tables after
	 each command line hierarchy has been processed.  */
      if (ent->fts_level == 0 && ent->fts_info == FTS_D && !print_totals)
	hash_clear (htab);

      process_file (fts, ent);
    }

  /* Ignore failure, since the only way it can do so is in failing to
     return to the original directory, and since we're about to exit,
     that doesn't matter.  */
  fts_close (fts);

  if (print_totals)
    print_size (tot_size, _("total"));

  return fail;
}

int
main (int argc, char **argv)
{
  int c;
  char *cwd_only[2];
  int max_depth_specified = 0;
  char **files;
  int fail;

  /* Bit flags that control how fts works.  */
  int bit_flags = FTS_PHYSICAL | FTS_TIGHT_CYCLE_CHECK;

  /* If nonzero, display only a total for each argument. */
  int opt_summarize_only = 0;

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

  fail = 0;
  while ((c = getopt_long (argc, argv, DEBUG_OPT "0abchHklmsxB:DLPSX:",
			   long_options, NULL)) != -1)
    {
      long int tmp_long;
      switch (c)
	{
	case 0:			/* Long option. */
	  break;

#if DU_DEBUG
	case 'd':
	  fts_debug = 1;
	  break;
#endif

	case '0':
	  opt_nul_terminate_output = true;
	  break;

	case 'a':
	  opt_all = 1;
	  break;

	case APPARENT_SIZE_OPTION:
	  apparent_size = 1;
	  break;

	case 'b':
	  apparent_size = 1;
	  human_output_opts = 0;
	  output_block_size = 1;
	  break;

	case 'c':
	  print_totals = 1;
	  break;

	case 'h':
	  human_output_opts = human_autoscale | human_SI | human_base_1024;
	  output_block_size = 1;
	  break;

	case 'H':
	  error (0, 0, _("WARNING: use --si, not -H; the meaning of the -H\
 option will soon\nchange to be the same as that of --dereference-args (-D)"));
	  /* fall through */
	case HUMAN_SI_OPTION:
	  human_output_opts = human_autoscale | human_SI;
	  output_block_size = 1;
	  break;

	case 'k':
	  human_output_opts = 0;
	  output_block_size = 1024;
	  break;

	case MAX_DEPTH_OPTION:		/* --max-depth=N */
	  if (xstrtol (optarg, NULL, 0, &tmp_long, NULL) == LONGINT_OK
	      && 0 <= tmp_long && tmp_long <= INT_MAX)
	    {
	      max_depth_specified = 1;
	      max_depth = (int) tmp_long;
	    }
	  else
	    {
	      error (0, 0, _("invalid maximum depth %s"),
		     quote (optarg));
	      fail = 1;
	    }
	  break;

	case 'm': /* obsolescent: FIXME: remove in 2005. */
	  human_output_opts = 0;
	  output_block_size = 1024 * 1024;
	  break;

	case 'l':
	  opt_count_all = 1;
	  break;

	case 's':
	  opt_summarize_only = 1;
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
	  opt_separate_dirs = 1;
	  break;

	case 'X':
	  if (add_exclude_file (add_exclude, exclude, optarg,
				EXCLUDE_WILDCARDS, '\n'))
	    {
	      error (0, errno, "%s", quotearg_colon (optarg));
	      fail = 1;
	    }
	  break;

	case EXCLUDE_OPTION:
	  add_exclude (exclude, optarg, EXCLUDE_WILDCARDS);
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  fail = 1;
	}
    }

  if (fail)
    usage (EXIT_FAILURE);

  if (opt_all && opt_summarize_only)
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
      error (0, 0,
	     _("warning: summarizing conflicts with --max-depth=%d"),
	       max_depth);
      usage (EXIT_FAILURE);
    }

  if (opt_summarize_only)
    max_depth = 0;

  files = (optind < argc ? argv + optind : cwd_only);

  /* Initialize the hash structure for inode numbers.  */
  hash_init ();

  exit (du_files (files, bit_flags) || G_fail
	? EXIT_FAILURE : EXIT_SUCCESS);
}
