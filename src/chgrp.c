/* chgrp -- change group ownership of files
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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "lchown.h"
#include "group-member.h"
#include "quote.h"
#include "xstrtol.h"
#include "chown-core.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "chgrp"

#define AUTHORS "David MacKenzie"

/* MAXUID may come from limits.h *or* sys/params.h (via system.h) above. */
#ifndef MAXUID
# define MAXUID UID_T_MAX
#endif
#ifndef MAXGID
# define MAXGID GID_T_MAX
#endif

#ifndef _POSIX_VERSION
struct group *getgrnam ();
#endif

#if ! HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

/* The name the program was run with. */
char *program_name;

/* The argument to the --reference option.  Use the group ID of this file.
   This file must exist.  */
static char *reference_file;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  REFERENCE_FILE_OPTION = CHAR_MAX + 1,
  DEREFERENCE_OPTION
};

static struct option const long_options[] =
{
  {"recursive", no_argument, 0, 'R'},
  {"changes", no_argument, 0, 'c'},
  {"dereference", no_argument, 0, DEREFERENCE_OPTION},
  {"no-dereference", no_argument, 0, 'h'},
  {"quiet", no_argument, 0, 'f'},
  {"silent", no_argument, 0, 'f'},
  {"reference", required_argument, 0, REFERENCE_FILE_OPTION},
  {"verbose", no_argument, 0, 'v'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {0, 0, 0, 0}
};

/* Set *G according to NAME. */

static void
parse_group (const char *name, gid_t *g)
{
  struct group *grp;

  if (*name == '\0')
    error (EXIT_FAILURE, 0, _("cannot change to null group"));

  grp = getgrnam (name);
  if (grp == NULL)
    {
      strtol_error s_err;
      unsigned long int tmp_long;

      if (!ISDIGIT (*name))
	error (EXIT_FAILURE, 0, _("invalid group name %s"), quote (name));

      s_err = xstrtoul (name, NULL, 0, &tmp_long, NULL);
      if (s_err != LONGINT_OK)
	STRTOL_FATAL_ERROR (name, _("group number"), s_err);

      if (tmp_long > MAXGID)
	error (EXIT_FAILURE, 0, _("invalid group number %s"), quote (name));

      *g = tmp_long;
    }
  else
    *g = grp->gr_gid;
  endgrent ();		/* Save a file descriptor. */
}

void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... GROUP FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name);
      fputs (_("\
Change the group membership of each FILE to GROUP.\n\
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
  -f, --silent, --quiet  suppress most error messages\n\
      --reference=RFILE  use RFILE's group rather than the specified\n\
                         GROUP value\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          output a diagnostic for every file processed\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  gid_t gid;
  int errors = 0;
  int optc;
  struct Chown_option chopt;

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

      chopt.group_name = gid_to_name (ref_stats.st_gid);
      gid = ref_stats.st_gid;
    }
  else
    {
      chopt.group_name = argv[optind++];
      parse_group (chopt.group_name, &gid);
    }

  for (; optind < argc; ++optind)
    errors |= change_file_owner (1, argv[optind], (uid_t) -1, gid,
				 (uid_t) -1, (gid_t) -1, &chopt);

  chopt_free (&chopt);

  exit (errors);
}
