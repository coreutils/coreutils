/* du -- summarize disk usage
   Copyright (C) 88, 89, 90, 91, 1995-2002 Free Software Foundation, Inc.

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
   * Additional options:
   -l		Count the size of all files, even if they have appeared
		already in another hard link.
   -x		Do not cross file-system boundaries during the recursion.
   -c		Write a grand total of all of the arguments after all
		arguments have been processed.  This can be used to find
		out the disk usage of a directory, with some files excluded.
   -h		Print sizes in human readable format (1k 234M 2G, etc).
   -H		Similar, but use powers of 1000 not 1024.
   -k		Print sizes in kilobytes.
   -m		Print sizes in megabytes.
   -b		Print sizes in bytes.
   -S		Count the size of each directory separately, not including
		the sizes of subdirectories.
   -D		Dereference only symbolic links given on the command line.
   -L		Dereference all symbolic links.
   --exclude=PATTERN Exclude files that match PATTERN.
   -X FILE	Exclude files that match patterns taken from FILE.

   By tege@sics.se, Torbjorn Granlund,
   and djm@ai.mit.edu, David MacKenzie.
   Variable blocks added by lm@sgi.com and eggert@twinsun.com.
*/

#include <config.h>
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <assert.h>

#include "system.h"
#include "error.h"
#include "exclude.h"
#include "hash.h"
#include "human.h"
#include "quote.h"
#include "same.h"
#include "save-cwd.h"
#include "savedir.h"
#include "xstrtol.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "du"

#define AUTHORS \
  "Torbjorn Granlund, David MacKenzie, Larry McVoy, and Paul Eggert"

/* Initial size of the hash table.  */
#define INITIAL_TABLE_SIZE 103

/* Initial size to allocate for `path'.  */
#define INITIAL_PATH_SIZE 100

/* Hash structure for inode and device numbers.  The separate entry
   structure makes it easier to rehash "in place".  */

struct entry
{
  ino_t st_ino;
  dev_t st_dev;
};

/* A set of dev/ino pairs.  */
static Hash_table *htab;

/* Structure for dynamically resizable strings. */

struct String
{
  unsigned alloc;		/* Size of allocation for the text.  */
  unsigned length;		/* Length of the text currently.  */
  char *text;			/* Pointer to the text.  */
};
typedef struct String String;

int stat ();
int lstat ();

/* Name under which this program was invoked.  */
char *program_name;

/* If nonzero, display counts for all files, not just directories. */
static int opt_all = 0;

/* If nonzero, count each hard link of files with multiple links. */
static int opt_count_all = 0;

/* If nonzero, do not cross file-system boundaries. */
static int opt_one_file_system = 0;

/* If nonzero, print a grand total at the end. */
static int print_totals = 0;

/* If nonzero, do not add sizes of subdirectories. */
static int opt_separate_dirs = 0;

/* If nonzero, dereference symlinks that are command line arguments. */
static int opt_dereference_arguments = 0;

/* Show the total for each directory (and file if --all) that is at
   most MAX_DEPTH levels down from the root of the hierarchy.  The root
   is at level 0, so `du --max-depth=0' is equivalent to `du -s'.  */
static int max_depth = INT_MAX;

/* If positive, the units to use when printing sizes;
   if negative, the human-readable base.  */
static int output_block_size;

/* Accumulated path for file or directory being processed.  */
static String *path;

/* A pointer to either lstat or stat, depending on whether
   dereferencing of all symbolic links is to be done. */
static int (*xstat) ();

/* The exit status to use if we don't get any fatal errors. */
static int exit_status;

/* File name patterns to exclude.  */
static struct exclude *exclude;

/* Grand total size of all args, in units of ST_NBLOCKSIZE-byte blocks. */
static uintmax_t tot_size = 0;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  EXCLUDE_OPTION = CHAR_MAX + 1,
  MAX_DEPTH_OPTION
};

static struct option const long_options[] =
{
  {"all", no_argument, NULL, 'a'},
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
  -B, --block-size=SIZE use SIZE-byte blocks\n\
  -b, --bytes           print size in bytes\n\
  -c, --total           produce a grand total\n\
  -D, --dereference-args  dereference PATHs when symbolic link\n\
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

/* Initialize string S1 to hold SIZE characters.  */

static void
str_init (String **s1, unsigned int size)
{
  String *s;

  s = (String *) xmalloc (sizeof (struct String));
  s->text = xmalloc (size + 1);

  s->alloc = size;
  *s1 = s;
}

static void
ensure_space (String *s, unsigned int size)
{
  if (s->alloc < size)
    {
      s->text = xrealloc (s->text, size + 1);
      s->alloc = size;
    }
}

/* Assign the null-terminated C-string CSTR to S1.  */

static void
str_copyc (String *s1, const char *cstr)
{
  unsigned l = strlen (cstr);
  ensure_space (s1, l);
  strcpy (s1->text, cstr);
  s1->length = l;
}

static void
str_concatc (String *s1, const char *cstr)
{
  unsigned l1 = s1->length;
  unsigned l2 = strlen (cstr);
  unsigned l = l1 + l2;

  ensure_space (s1, l);
  strcpy (s1->text + l1, cstr);
  s1->length = l;
}

/* Truncate the string S1 to have length LENGTH.  */

static void
str_trunc (String *s1, unsigned int length)
{
  if (s1->length > length)
    {
      s1->text[length] = 0;
      s1->length = length;
    }
}

/* Print N_BLOCKS followed by STRING on a line.  NBLOCKS is the number of
   ST_NBLOCKSIZE-byte blocks; convert it to OUTPUT_BLOCK_SIZE units before
   printing.  If OUTPUT_BLOCK_SIZE is negative, use a human readable
   notation instead.  */

static void
print_size (uintmax_t n_blocks, const char *string)
{
  char buf[LONGEST_HUMAN_READABLE + 1];
  printf ("%s\t%s\n",
	  human_readable_inexact (n_blocks, buf, ST_NBLOCKSIZE,
				  output_block_size, human_ceiling),
	  string);
  fflush (stdout);
}

/* Restore the previous working directory or exit.
   If CWD is null, simply call `chdir ("..")'.  Otherwise,
   use CWD and free it.  CURR_DIR_NAME is the name of the current directory
   and is used solely in failure diagnostics.  */

static void
pop_dir (struct saved_cwd *cwd, const char *curr_dir_name)
{
  if (cwd)
    {
      if (restore_cwd (cwd, "..", curr_dir_name))
	exit (EXIT_FAILURE);
      free_cwd (cwd);
    }
  else if (chdir ("..") < 0)
    {
      error (EXIT_FAILURE, errno, _("cannot change to `..' from directory %s"),
	     quote (curr_dir_name));
    }
}

/* Print (if appropriate) the size (in units determined by `output_block_size')
   of file or directory ENT. Return the size of ENT in units of 512-byte
   blocks.  TOP is one for external calls, zero for recursive calls.
   LAST_DEV is the device that the parent directory of ENT is on.
   DEPTH is the number of levels (in hierarchy) down from a command
   line argument.  Don't print if DEPTH > max_depth.
   An important invariant is that when this function returns, the current
   working directory is the same as when it was called.  */

static uintmax_t
count_entry (const char *ent, int top, dev_t last_dev, int depth)
{
  uintmax_t size;
  struct stat stat_buf;

  if (((top && opt_dereference_arguments)
       ? stat (ent, &stat_buf)
       : (*xstat) (ent, &stat_buf)) < 0)
    {
      error (0, errno, "%s", quote (path->text));
      exit_status = 1;
      return 0;
    }

  if (!opt_count_all
      && stat_buf.st_nlink > 1
      && hash_ins (stat_buf.st_ino, stat_buf.st_dev))
    return 0;			/* Have counted this already.  */

  size = ST_NBLOCKS (stat_buf);
  tot_size += size;

  if (S_ISDIR (stat_buf.st_mode))
    {
      unsigned pathlen;
      dev_t dir_dev;
      char *name_space;
      char *namep;
      struct saved_cwd *cwd;
      struct saved_cwd cwd_buf;
      struct stat e_buf;

      dir_dev = stat_buf.st_dev;

      /* Return `0' here, not SIZE, since the SIZE bytes
	 would reside in the new filesystem.  */
      if (opt_one_file_system && !top && last_dev != dir_dev)
	return 0;		/* Don't enter a new file system.  */

#ifndef S_ISLNK
# define S_ISLNK(s) 0
#endif
      /* If we're traversing more than one level, or if we're
	 dereferencing symlinks and we're about to chdir through a
	 symlink, remember the current directory so we can return to
	 it later.  In other cases, chdir ("..") works fine.
	 Treat `.' and `..' like multi-level paths, since `chdir ("..")'
	 wont't restore the current working directory after a `chdir'
	 to one of those.  */
      if (strchr (ent, '/')
	  || DOT_OR_DOTDOT (ent)
	  || (xstat == stat
	      && lstat (ent, &e_buf) == 0
	      && S_ISLNK (e_buf.st_mode)))
	{
	  if (save_cwd (&cwd_buf))
	    exit (EXIT_FAILURE);
	  cwd = &cwd_buf;
	}
      else
	cwd = NULL;

      if (chdir (ent) < 0)
	{
	  error (0, errno, _("cannot change to directory %s"),
		 quote (path->text));
	  if (cwd)
	    free_cwd (cwd);
	  exit_status = 1;
	  /* Do return SIZE, here, since even though we can't chdir into ENT,
	     we *can* count the blocks used by its directory entry.  */
	  return opt_separate_dirs ? 0 : size;
	}

      name_space = savedir (".");
      if (name_space == NULL)
	{
	  error (0, errno, "%s", quote (path->text));
	  pop_dir (cwd, path->text);
	  exit_status = 1;
	  /* Do count the SIZE bytes.  */
	  return opt_separate_dirs ? 0 : size;
	}

      /* Remember the current path.  */

      str_concatc (path, "/");
      pathlen = path->length;

      for (namep = name_space; *namep; namep += strlen (namep) + 1)
	{
	  if (!excluded_filename (exclude, namep))
	    {
	      str_concatc (path, namep);
	      size += count_entry (namep, 0, dir_dev, depth + 1);
	      str_trunc (path, pathlen);
	    }
	}

      free (name_space);
      pop_dir (cwd, path->text);

      str_trunc (path, pathlen - 1); /* Remove the "/" we added.  */
      if (depth <= max_depth || top)
	print_size (size, path->length > 0 ? path->text : "/");
      return opt_separate_dirs ? 0 : size;
    }
  else if ((opt_all && depth <= max_depth) || top)
    {
      /* FIXME: make this an option.  */
      int print_only_dir_size = 0;
      if (!print_only_dir_size)
	print_size (size, path->length > 0 ? path->text : "/");
    }

  return size;
}

/* Recursively print the sizes of the directories (and, if selected, files)
   named in FILES, the last entry of which is NULL.  */

static void
du_files (char **files)
{
  int i;			/* Index in FILES. */

  for (i = 0; files[i]; i++)
    {
      char *arg;
      int s;

      arg = files[i];

      /* Delete final slash in the argument, unless the slash is alone.  */
      s = strlen (arg) - 1;
      if (s != 0)
	{
	  if (arg[s] == '/')
	    arg[s] = 0;

	  str_copyc (path, arg);
	}
      else if (arg[0] == '/')
	str_trunc (path, 0);	/* Null path for root directory.  */
      else
	str_copyc (path, arg);

      if (!print_totals)
	hash_clear (htab);

      count_entry (arg, 1, 0, 0);
    }

  if (print_totals)
    print_size (tot_size, _("total"));
}

int
main (int argc, char **argv)
{
  int c;
  char *cwd_only[2];
  int max_depth_specified = 0;

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
  xstat = lstat;

  human_block_size (getenv ("DU_BLOCK_SIZE"), 0, &output_block_size);

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

	case 'b':
	  output_block_size = 1;
	  break;

	case 'c':
	  print_totals = 1;
	  break;

	case 'h':
	  output_block_size = -1024;
	  break;

	case 'H':
	  output_block_size = -1000;
	  break;

	case 'k':
	  output_block_size = 1024;
	  break;

	case MAX_DEPTH_OPTION:		/* --max-depth=N */
	  if (xstrtol (optarg, NULL, 0, &tmp_long, NULL) != LONGINT_OK
	      || tmp_long < 0 || tmp_long > INT_MAX)
	    error (EXIT_FAILURE, 0, _("invalid maximum depth %s"), quote (optarg));

	  max_depth_specified = 1;
	  max_depth = (int) tmp_long;
 	  break;

	case 'm': /* obsolescent */
	  output_block_size = 1024 * 1024;
	  break;

	case 'l':
	  opt_count_all = 1;
	  break;

	case 's':
	  opt_summarize_only = 1;
	  break;

	case 'x':
	  opt_one_file_system = 1;
	  break;

	case 'B':
	  human_block_size (optarg, 1, &output_block_size);
	  break;

	case 'D':
	  opt_dereference_arguments = 1;
	  break;

	case 'L':
	  xstat = stat;
	  break;

	case 'S':
	  opt_separate_dirs = 1;
	  break;

	case 'X':
	  if (add_exclude_file (add_exclude, exclude, optarg,
				EXCLUDE_WILDCARDS, '\n'))
	    error (EXIT_FAILURE, errno, "%s", quote (optarg));
	  break;

	case EXCLUDE_OPTION:
	  add_exclude (exclude, optarg, EXCLUDE_WILDCARDS);
	  break;

	case_GETOPT_HELP_CHAR;

	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

	default:
	  usage (EXIT_FAILURE);
	}
    }

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

  /* Initialize the hash structure for inode numbers.  */
  hash_init ();

  str_init (&path, INITIAL_PATH_SIZE);

  du_files (optind == argc ? cwd_only : argv + optind);

  exit (exit_status);
}
