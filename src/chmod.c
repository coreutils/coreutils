/* chmod -- change permission modes of files
   Copyright (C) 1989, 1990, 1991 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Options:
   -R	Recursively change modes of directory contents.
   -c	Verbosely describe only files whose modes actually change.
   -f	Do not print error messages about files.
   -v	Verbosely describe changed modes.

   David MacKenzie <djm@gnu.ai.mit.edu> */

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include "modechange.h"
#include "system.h"
#include "version.h"

int lstat ();

char *savedir ();
char *xmalloc ();
char *xrealloc ();
void error ();
void mode_string ();

static int change_file_mode ();
static int change_dir_mode ();
static void describe_change ();
static void usage ();

/* The name the program was run with. */
char *program_name;

/* If nonzero, change the modes of directories recursively. */
static int recurse;

/* If nonzero, force silence (no error messages). */
static int force_silent;

/* If nonzero, describe the modes we set. */
static int verbose;

/* If nonzero, describe only modes that change. */
static int changes_only;

/* If non-zero, display usage information and exit.  */
static int flag_help;

/* If non-zero, print the version on standard error.  */
static int flag_version;

static struct option const long_options[] =
{
  {"recursive", no_argument, 0, 'R'},
  {"changes", no_argument, 0, 'c'},
  {"silent", no_argument, 0, 'f'},
  {"quiet", no_argument, 0, 'f'},
  {"verbose", no_argument, 0, 'v'},
  {"help", no_argument, &flag_help, 1},
  {"version", no_argument, &flag_version, 1},
  {0, 0, 0, 0}
};

/* Parse the ASCII mode given on the command line into a linked list
   of `struct mode_change' and apply that to each file argument. */

void
main (argc, argv)
     int argc;
     char **argv;
{
  struct mode_change *changes;
  int errors = 0;
  int modeind = 0;		/* Index of the mode argument in `argv'. */
  int thisind;
  int c;

  program_name = argv[0];
  recurse = force_silent = verbose = changes_only = 0;

  while (1)
    {
      thisind = optind ? optind : 1;

      c = getopt_long (argc, argv, "RcfvrwxXstugoa,+-=", long_options,
		       (int *) 0);
      if (c == EOF)
	break;

      switch (c)
	{
	case 0:
	  break;
	case 'r':
	case 'w':
	case 'x':
	case 'X':
	case 's':
	case 't':
	case 'u':
	case 'g':
	case 'o':
	case 'a':
	case ',':
	case '+':
	case '-':
	case '=':
	  if (modeind != 0 && modeind != thisind)
	    error (1, 0, "invalid mode");
	  modeind = thisind;
	  break;
	case 'R':
	  recurse = 1;
	  break;
	case 'c':
	  verbose = 1;
	  changes_only = 1;
	  break;
	case 'f':
	  force_silent = 1;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	default:
	  usage ();
	}
    }

  if (flag_version)
    {
      fprintf (stderr, "%s\n", version_string);
      exit (0);
    }

  if (flag_help)
    usage ();

  if (modeind == 0)
    modeind = optind++;

  if (optind >= argc)
    usage ();

  changes = mode_compile (argv[modeind],
			  MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
  if (changes == MODE_INVALID)
    error (1, 0, "invalid mode");
  else if (changes == MODE_MEMORY_EXHAUSTED)
    error (1, 0, "virtual memory exhausted");

  for (; optind < argc; ++optind)
    errors |= change_file_mode (argv[optind], changes);

  exit (errors);
}

/* Change the mode of FILE according to the list of operations CHANGES.
   Return 0 if successful, 1 if errors occurred. */

static int
change_file_mode (file, changes)
     char *file;
     struct mode_change *changes;
{
  struct stat file_stats;
  unsigned short newmode;
  int errors = 0;

  if (lstat (file, &file_stats))
    {
      if (force_silent == 0)
	error (0, errno, "%s", file);
      return 1;
    }
#ifdef S_ISLNK
  if (S_ISLNK (file_stats.st_mode))
    return 0;
#endif

  newmode = mode_adjust (file_stats.st_mode, changes);

  if (newmode != (file_stats.st_mode & 07777))
    {
      if (verbose)
	describe_change (file, newmode, 1);
      if (chmod (file, (int) newmode))
	{
	  if (force_silent == 0)
	    error (0, errno, "%s", file);
	  errors = 1;
	}
    }
  else if (verbose && changes_only == 0)
    describe_change (file, newmode, 0);

  if (recurse && S_ISDIR (file_stats.st_mode))
    errors |= change_dir_mode (file, changes, &file_stats);
  return errors;
}

/* Recursively change the modes of the files in directory DIR
   according to the list of operations CHANGES.
   STATP points to the results of lstat on DIR.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_mode (dir, changes, statp)
     char *dir;
     struct mode_change *changes;
     struct stat *statp;
{
  char *name_space, *namep;
  char *path;			/* Full path of each entry to process. */
  unsigned dirlength;		/* Length of DIR and '\0'. */
  unsigned filelength;		/* Length of each pathname to process. */
  unsigned pathlength;		/* Bytes allocated for `path'. */
  int errors = 0;

  errno = 0;
  name_space = savedir (dir, statp->st_size);
  if (name_space == NULL)
    {
      if (errno)
	{
	  if (force_silent == 0)
	    error (0, errno, "%s", dir);
	  return 1;
	}
      else
	error (1, 0, "virtual memory exhausted");
    }

  dirlength = strlen (dir) + 1;	/* + 1 is for the trailing '/'. */
  pathlength = dirlength + 1;
  /* Give `path' a dummy value; it will be reallocated before first use. */
  path = xmalloc (pathlength);
  strcpy (path, dir);
  path[dirlength - 1] = '/';

  for (namep = name_space; *namep; namep += filelength - dirlength)
    {
      filelength = dirlength + strlen (namep) + 1;
      if (filelength > pathlength)
	{
	  pathlength = filelength * 2;
	  path = xrealloc (path, pathlength);
	}
      strcpy (path + dirlength, namep);
      errors |= change_file_mode (path, changes);
    }
  free (path);
  free (name_space);
  return errors;
}

/* Tell the user the mode MODE that file FILE has been set to;
   if CHANGED is zero, FILE had that mode already. */

static void
describe_change (file, mode, changed)
     char *file;
     unsigned short mode;
     int changed;
{
  char perms[11];		/* "-rwxrwxrwx" ls-style modes. */

  mode_string (mode, perms);
  perms[10] = '\0';		/* `mode_string' does not null terminate. */
  if (changed)
    printf ("mode of %s changed to %04o (%s)\n",
	    file, mode & 07777, &perms[1]);
  else
    printf ("mode of %s retained as %04o (%s)\n",
	    file, mode & 07777, &perms[1]);
}

static void
usage ()
{
  fprintf (stderr, "\
Usage: %s [-Rcfv] [--recursive] [--changes] [--silent] [--quiet]\n\
       [--verbose] [--help] [--version] mode file...\n\
       mode is [ugoa...][[+-=][rwxXstugo...]...][,...] or octal number\n",
	   program_name);
  exit (1);
}
