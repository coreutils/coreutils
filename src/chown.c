/* chown -- change user and group ownership of files
   Copyright (C) 89, 90, 91, 1995-2003 Free Software Foundation, Inc.

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

/*
              |		              user
              | unchanged                 explicit
 -------------|-------------------------+-------------------------|
 g unchanged  | ---                     | chown u		  |
 r            |-------------------------+-------------------------|
 o explicit   | chgrp g or chown .g     | chown u.g		  |
 u            |-------------------------+-------------------------|
 p from passwd| ---			| chown u.		  |
              |-------------------------+-------------------------|

   Written by David MacKenzie <djm@gnu.ai.mit.edu>. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "chown-core.h"
#include "error.h"
#include "fts_.h"
#include "lchown.h"
#include "quote.h"
#include "userspec.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "chown"

#define WRITTEN_BY _("Written by David MacKenzie and Jim Meyering.")

/* The name the program was run with. */
char *program_name;

/* The argument to the --reference option.  Use the owner and group IDs
   of this file.  This file must exist.  */
static char *reference_file;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  FROM_OPTION = CHAR_MAX + 1,
  DEREFERENCE_OPTION,
  REFERENCE_FILE_OPTION
};

static struct option const long_options[] =
{
  {"recursive", no_argument, 0, 'R'},
  {"changes", no_argument, 0, 'c'},
  {"dereference", no_argument, 0, DEREFERENCE_OPTION},
  {"from", required_argument, 0, FROM_OPTION},
  {"no-dereference", no_argument, 0, 'h'},
  {"quiet", no_argument, 0, 'f'},
  {"silent", no_argument, 0, 'f'},
  {"reference", required_argument, 0, REFERENCE_FILE_OPTION},
  {"verbose", no_argument, 0, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0}
};

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... OWNER[:[GROUP]] FILE...\n\
  or:  %s [OPTION]... :GROUP FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name, program_name);
      fputs (_("\
Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\
With --reference, change the owner and group of each FILE to those of RFILE.\n\
\n\
  -c, --changes          like verbose but report only when a change is made\n\
      --dereference      affect the referent of each symbolic link, rather\n\
                         than the symbolic link itself\n\
"), stdout);
      fputs (_("\
  -h, --no-dereference   affect each symbolic link instead of any referenced\n\
                         file (useful only on systems that can change the\n\
                         ownership of a symlink)\n\
"), stdout);
      fputs (_("\
      --from=CURRENT_OWNER:CURRENT_GROUP\n\
                         change the owner and/or group of each file only if\n\
                         its current owner and/or group match those specified\n\
                         here.  Either may be omitted, in which case a match\n\
                         is not required for the omitted attribute.\n\
"), stdout);
      fputs (_("\
  -f, --silent, --quiet  suppress most error messages\n\
      --reference=RFILE  use RFILE's owner and group rather than\n\
                         the specifying OWNER:GROUP values\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          output a diagnostic for every file processed\n\
\n\
"), stdout);
      fputs (_("\
The following options modify how a hierarchy is traversed when the -R\n\
option is also specified.  If more than one is specified, only the final\n\
one takes effect.\n\
\n\
  -H                     if a command line argument is a symbolic link\n\
                         to a directory, traverse it\n\
  -L                     traverse every symbolic link to a directory\n\
                         encountered\n\
  -P                     do not traverse any symbolic links (default)\n\
\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Owner is unchanged if missing.  Group is unchanged if missing, but changed\n\
to login group if implied by a `:'.  OWNER and GROUP may be numeric as well\n\
as symbolic.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  uid_t uid = (uid_t) -1;	/* New uid; -1 if not to be changed. */
  gid_t gid = (uid_t) -1;	/* New gid; -1 if not to be changed. */
  uid_t old_uid = (uid_t) -1;	/* Old uid; -1 if unrestricted. */
  gid_t old_gid = (uid_t) -1;	/* Old gid; -1 if unrestricted. */
  /* Bit flags that control how fts works.  */
  int bit_flags = FTS_PHYSICAL;
  struct Chown_option chopt;
  int fail = 0;
  int optc;

  initialize_main (&argc, &argv);
  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  chopt_init (&chopt);

  while ((optc = getopt_long (argc, argv, "HLPRcfhv", long_options, NULL))
	 != -1)
    {
      switch (optc)
	{
	case 0:
	  break;

	case 'H': /* Traverse command-line symlinks-to-directories.  */
	  bit_flags |= FTS_COMFOLLOW;
	  break;

	case 'L': /* Traverse all symlinks-to-directories.  */
	  bit_flags &= ~FTS_PHYSICAL;
	  bit_flags |= FTS_LOGICAL;
	  break;

	case 'P': /* Traverse no symlinks-to-directories.  */
	  bit_flags |= FTS_PHYSICAL;
	  bit_flags &= ~FTS_LOGICAL;
	  bit_flags &= ~FTS_COMFOLLOW;
	  break;

	case 'h': /* --no-dereference: affect symlinks */
	  chopt.affect_symlink_referent = false;
	  break;

	case DEREFERENCE_OPTION: /* --dereference: affect the referent
				    of each symlink */
	  chopt.affect_symlink_referent = true;
	  break;

	case REFERENCE_FILE_OPTION:
	  reference_file = optarg;
	  break;

	case FROM_OPTION:
	  {
	    char *u_dummy, *g_dummy;
	    const char *e = parse_user_spec (optarg,
					     &old_uid, &old_gid,
					     &u_dummy, &g_dummy);
	    if (e)
	      error (EXIT_FAILURE, 0, "%s: %s", quote (optarg), e);
	    break;
	  }

	case 'R':
	  chopt.recurse = true;
	  break;

	case 'c':
	  chopt.verbosity = V_changes_only;
	  break;

	case 'f':
	  chopt.force_silent = true;
	  break;

	case 'v':
	  chopt.verbosity = V_high;
	  break;

	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, WRITTEN_BY);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (argc - optind + (reference_file ? 1 : 0) <= 1)
    {
      error (0, 0, _("too few arguments"));
      usage (EXIT_FAILURE);
    }

  if (reference_file)
    {
      struct stat ref_stats;
      if (stat (reference_file, &ref_stats))
	error (EXIT_FAILURE, errno, _("failed to get attributes of %s"),
	       quote (reference_file));

      uid = ref_stats.st_uid;
      gid = ref_stats.st_gid;
      chopt.user_name = uid_to_name (ref_stats.st_uid);
      chopt.group_name = gid_to_name (ref_stats.st_gid);
    }
  else
    {
      const char *e = parse_user_spec (argv[optind], &uid, &gid,
				       &chopt.user_name, &chopt.group_name);
      if (e)
        error (EXIT_FAILURE, 0, "%s: %s", quote (argv[optind]), e);

      /* FIXME: set it to the empty string?  */
      if (chopt.user_name == NULL)
        chopt.user_name = "";

      optind++;
    }

  fail = chown_files (argv + optind, bit_flags,
		      uid, gid,
		      old_uid, old_gid, &chopt);

  chopt_free (&chopt);

  exit (fail ? EXIT_FAILURE : EXIT_SUCCESS);
}
