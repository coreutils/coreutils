/* du -- summarize disk usage
   Copyright (C) 1988-1991, 1995-2003 Free Software Foundation, Inc.

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
   Rewritten to use nftw by Jim Meyering.  */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "dirname.h" /* for strip_trailing_slashes */
#include "error.h"
#include "exclude.h"
#include "ftw.h"
#include "hash.h"
#include "human.h"
#include "quote.h"
#include "quotearg.h"
#include "same.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "du"

#define AUTHORS \
  N_ ("Torbjorn Granlund, David MacKenzie, Larry McVoy, Paul Eggert, and Jim Meyering")

/* Initial size of the hash table.  */
#define INITIAL_TABLE_SIZE 103

/* The maximum number of simultaneously open file handles that
   may be used by ftw.  */
#define MAX_N_DESCRIPTORS (3 * UTILS_OPEN_MAX / 4)

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

/* If nonzero, print a grand total at the end.  */
static int print_totals = 0;

/* If nonzero, do not add sizes of subdirectories.  */
static int opt_separate_dirs = 0;

/* If nonzero, dereference symlinks that are command line arguments.
   Implementing this while still using nftw is a little tricky.
   For each command line argument that is a symlink-to-directory,
   call nftw with "command_line_arg/." and remember to omit the
   added `/.' when printing.  */
static int opt_dereference_arguments = 0;

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

/* In some cases, we have to append `/.' or just `.' to an argument
   (to dereference a symlink).  When we do that, we don't want to
   expose this artifact when printing file/directory names, so these
   variables keep track of the length of the original command line
   argument and the length of the suffix we've added, respectively.
   ARG_LENGTH == 0 indicates that we haven't added a suffix.
   This information is used to omit any such added characters when
   printing names.  */
size_t arg_length;
size_t suffix_length;

/* Nonzero indicates that du should exit with EXIT_FAILURE upon completion.  */
int G_fail;

#define IS_FTW_DIR_TYPE(Type)	\
  ((Type) == FTW_D		\
   || (Type) == FTW_DP		\
   || (Type) == FTW_DNR)

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  APPARENT_SIZE_OPTION = CHAR_MAX + 1,
  EXCLUDE_OPTION,
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
  {"si", no_argument, 0, 'H'},
  {"kilobytes", no_argument, NULL, 'k'}, /* long form is obsolescent */
  {"max-depth", required_argument, NULL, MAX_DEPTH_OPTION},
  {"megabytes", no_argument, NULL, 'm'}, /* obsolescent */
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
  if (status != 0)
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
  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n\
  -H, --si              likewise, but use powers of 1000 not 1024\n\
  -k                    like --block-size=1K\n\
  -l, --count-links     count sizes many times if hard linked\n\
"), stdout);
      fputs (_("\
  -L, --dereference     dereference all symbolic links\n\
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
kB 1000, K 1024, MB 1,000,000, M 1,048,576, and so on for G, T, P, E, Z, Y.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

static unsigned int
entry_hash (void const *x, unsigned int table_size)
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

  ent = (struct entry *) xmalloc (sizeof *ent);
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
  printf ("\t%s\n", string);
  fflush (stdout);
}

/* This function is called once for every file system object that nftw
   encounters.  nftw does a depth-first traversal.  This function knows
   that and accumulates per-directory totals based on changes in
   the depth of the current entry.  */

static int
process_file (const char *file, const struct stat *sb, int file_type,
	      struct FTW *info)
{
  uintmax_t size;
  uintmax_t size_to_print;
  static int first_call = 1;
  static size_t prev_level;
  static size_t n_alloc;
  static uintmax_t *sum_ent;
  static uintmax_t *sum_subdir;
  int print = 1;

  /* Always define info->skip before returning.  */
  info->skip = excluded_filename (exclude, file + info->base);

  switch (file_type)
    {
    case FTW_NS:
      error (0, errno, _("cannot access %s"), quote (file));
      G_fail = 1;
      return 0;

    case FTW_DCHP:
      error (0, errno, _("cannot change to parent of directory %s"),
	     quote (file));
      G_fail = 1;
      return 0;

    case FTW_DCH:
      /* Don't return just yet, since although nftw couldn't chdir into the
	 directory, it was able to stat it, so we do have a size.  */
      error (0, errno, _("cannot change to directory %s"), quote (file));
      G_fail = 1;
      break;

    case FTW_DNR:
      /* Don't return just yet, since although nftw couldn't read the
	 directory, it was able to stat it, so we do have a size.  */
      error (0, errno, _("cannot read directory %s"), quote (file));
      G_fail = 1;
      break;

    default:
      break;
    }

  /* If this is the first (pre-order) encounter with a directory,
     return right away.  */
  if (file_type == FTW_DPRE)
    return 0;

  /* If the file is being excluded or if it has already been counted
     via a hard link, then don't let it contribute to the sums.  */
  if (info->skip
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
      n_alloc = info->level + 10;
      sum_ent = XCALLOC (uintmax_t, n_alloc);
      sum_subdir = XCALLOC (uintmax_t, n_alloc);
    }
  else
    {
      /* FIXME: it's a shame that we need these `size_t' casts to avoid
	 warnings from gcc about `comparison between signed and unsigned'.
	 Probably unavoidable, assuming that the members of struct FTW
	 are of type `int' (historical), since I want variables like
	 n_alloc and prev_level to have types that make sense.  */
      if (n_alloc <= (size_t) info->level)
	{
	  n_alloc = info->level * 2;
	  sum_ent = XREALLOC (sum_ent, uintmax_t, n_alloc);
	  sum_subdir = XREALLOC (sum_subdir, uintmax_t, n_alloc);
	}
    }

  size_to_print = size;

  if (! first_call)
    {
      if ((size_t) info->level == prev_level)
	{
	  /* This is usually the most common case.  Do nothing.  */
	}
      else if ((size_t) info->level > prev_level)
	{
	  /* Descending the hierarchy.
	     Clear the accumulators for *all* levels between prev_level
	     and the current one.  The depth may change dramatically,
	     e.g., from 1 to 10.  */
	  int i;
	  for (i = prev_level + 1; i <= info->level; i++)
	    sum_ent[i] = sum_subdir[i] = 0;
	}
      else /* info->level < prev_level */
	{
	  /* Ascending the hierarchy.
	     nftw processes a directory only after all entries in that
	     directory have been processed.  When the depth decreases,
	     propagate sums from the children (prev_level) to the parent.
	     Here, the current level is always one smaller than the
	     previous one.  */
	  assert ((size_t) info->level == prev_level - 1);
	  size_to_print += sum_ent[prev_level];
	  if (!opt_separate_dirs)
	    size_to_print += sum_subdir[prev_level];
	  sum_subdir[info->level] += (sum_ent[prev_level]
				      + sum_subdir[prev_level]);
	}
    }

  prev_level = info->level;
  first_call = 0;

  /* Let the size of a directory entry contribute to the total for the
     containing directory, unless --separate-dirs (-S) is specified.  */
  if ( ! (opt_separate_dirs && IS_FTW_DIR_TYPE (file_type)))
    sum_ent[info->level] += size;

  /* Even if this directory is unreadable or we can't chdir into it,
     do let its size contribute to the total, ... */
  tot_size += size;

  /* ... but don't print out a total for it, since without the size(s)
     of any potential entries, it could be very misleading.  */
  if (file_type == FTW_DNR || file_type == FTW_DCH)
    return 0;

  /* If we're not counting an entry, e.g., because it's a hard link
     to a file we've already counted (and --count-links), then don't
     print a line for it.  */
  if (!print)
    return 0;

  /* FIXME: This looks suspiciously like it could be simplified.  */
  if ((IS_FTW_DIR_TYPE (file_type) &&
		     (info->level <= max_depth || info->level == 0))
      || ((opt_all && info->level <= max_depth) || info->level == 0))
    {
      print_only_size (size_to_print);
      fputc ('\t', stdout);
      if (arg_length)
	{
	  /* Print the file name, but without the `.' or `/.'
	     directory suffix that we may have added in main.  */
	  /* Print everything before the part we appended.  */
	  fwrite (file, arg_length, 1, stdout);
	  /* Print everything after what we appended.  */
	  fputs (file + arg_length + suffix_length
		 + (file[arg_length + suffix_length] == '/'), stdout);
	}
      else
	{
	  fputs (file, stdout);
	}
      fputc ('\n', stdout);
      fflush (stdout);
    }

  return 0;
}

static int
is_symlink_to_dir (char const *file)
{
  char *f;
  struct stat sb;

  ASSIGN_STRDUPA (f, file);
  strip_trailing_slashes (f);
  return (lstat (f, &sb) == 0 && S_ISLNK (sb.st_mode)
	  && stat (f, &sb) == 0 && S_ISDIR (sb.st_mode));
}

/* Recursively print the sizes of the directories (and, if selected, files)
   named in FILES, the last entry of which is NULL.
   FTW_FLAGS controls how nftw works.
   Return nonzero upon error.  */

static int
du_files (char **files, int ftw_flags)
{
  int fail = 0;
  int i;
  for (i = 0; files[i]; i++)
    {
      char *file = files[i];
      char *orig = file;
      int err;
      arg_length = 0;

      if (!print_totals)
	hash_clear (htab);

      /* When dereferencing only command line arguments, we're using
	 nftw's FTW_PHYS flag, so a symlink-to-directory specified on
	 the command line wouldn't normally be dereferenced.  To work
	 around that, we incur the overhead of appending `/.' (or `.')
	 now, and later removing it each time we output the name of
	 a derived file or directory name.  */
      if (opt_dereference_arguments && is_symlink_to_dir (file))
	{
	  size_t len = strlen (file);
	  /* Append `/.', but if there's already a trailing slash,
	     append only the `.'.  */
	  char const *suffix = (file[len - 1] == '/' ? "." : "/.");
	  char *new_file;
	  suffix_length = strlen (suffix);
	  new_file = xmalloc (len + suffix_length + 1);
	  memcpy (mempcpy (new_file, file, len), suffix, suffix_length + 1);
	  arg_length = len;
	  file = new_file;
	}

      err = nftw (file, process_file, MAX_N_DESCRIPTORS, ftw_flags);
      if (err)
	error (0, errno, "%s", quote (orig));
      fail |= err;

      if (arg_length)
	free (file);
    }

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

  /* Bit flags that control how nftw works.  */
  int ftw_flags = FTW_DEPTH | FTW_PHYS | FTW_CHDIR;

  /* If nonzero, display only a total for each argument. */
  int opt_summarize_only = 0;

  cwd_only[0] = ".";
  cwd_only[1] = NULL;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  exclude = new_exclude ();

  human_output_opts = human_options (getenv ("DU_BLOCK_SIZE"), false,
				     &output_block_size);

  fail = 0;
  while ((c = getopt_long (argc, argv, "abchHklmsxB:DLSX:", long_options, NULL))
	 != -1)
    {
      long int tmp_long;
      switch (c)
	{
	case 0:			/* Long option. */
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

	case 'm': /* obsolescent */
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
	  ftw_flags |= FTW_MOUNT;
	  break;

	case 'B':
	  human_output_opts = human_options (optarg, true, &output_block_size);
	  break;

	case 'D':
	  opt_dereference_arguments = 1;
	  break;

	case 'L':
	  ftw_flags &= ~FTW_PHYS;
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

  files = (optind == argc ? cwd_only : argv + optind);

  /* Initialize the hash structure for inode numbers.  */
  hash_init ();

  exit (du_files (files, ftw_flags) || G_fail
	? EXIT_FAILURE : EXIT_SUCCESS);
}
