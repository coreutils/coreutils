/* chown -- change user and group ownership of files
   Copyright (C) 89, 90, 91, 1995-2002 Free Software Foundation, Inc.

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
              |     		      user
              | unchanged                 explicit
 -------------|-------------------------+-------------------------|
 g unchanged  | ---                     | chown u 		  |
 r            |-------------------------+-------------------------|
 o explicit   | chgrp g or chown .g     | chown u.g		  |
 u            |-------------------------+-------------------------|
 p from passwd| ---      	        | chown u.       	  |
              |-------------------------+-------------------------|

   Written by David MacKenzie <djm@gnu.ai.mit.edu>. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "lchown.h"
#include "quote.h"
#include "chown-core.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "chown"

#define AUTHORS "David MacKenzie"

#ifndef _POSIX_VERSION
struct passwd *getpwnam ();
struct group *getgrnam ();
struct group *getgrgid ();
#endif

#if ! HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

char *parse_user_spec ();

/* The name the program was run with. */
char *program_name;

/* The argument to the --reference option.  Use the owner and group IDs
   of this file.  This file must exist.  */
static char *reference_file;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  REFERENCE_FILE_OPTION = CHAR_MAX + 1,
  DEREFERENCE_OPTION,
  FROM_OPTION
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
\n\
  -c, --changes          like verbose but report only when a change is made\n\
      --dereference      affect the referent of each symbolic link, rather\n\
                         than the symbolic link itself\n\
"), stdout);
      fputs (_("\
  -h, --no-dereference   affect symbolic links instead of any referenced file\n\
                         (available only on systems that can change the\n\
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
                         the specified OWNER:GROUP values\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          output a diagnostic for every file processed\n\
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
  struct Chown_option chopt;

  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  chopt_init (&chopt);

  while ((optc = getopt_long (argc, argv, "Rcfhv", long_options, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case REFERENCE_FILE_OPTION:
	  reference_file = optarg;
	  break;
	case DEREFERENCE_OPTION:
	  chopt.dereference = DEREF_ALWAYS;
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
	  chopt.recurse = 1;
	  break;
	case 'c':
	  chopt.verbosity = V_changes_only;
	  break;
	case 'f':
	  chopt.force_silent = 1;
	  break;
	case 'h':
	  chopt.dereference = DEREF_NEVER;
	  break;
	case 'v':
	  chopt.verbosity = V_high;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
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

  for (; optind < argc; ++optind)
    errors |= change_file_owner (1, argv[optind], uid, gid,
				 old_uid, old_gid, &chopt);

  chopt_free (&chopt);

  exit (errors);
}
