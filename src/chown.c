/* chown -- change user and group ownership of files
   Copyright (C) 89, 90, 91, 95, 96, 1997 Free Software Foundation, Inc.

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
#include <pwd.h>
#include <grp.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "savedir.h"

#ifndef _POSIX_VERSION
struct passwd *getpwnam ();
struct group *getgrnam ();
struct group *getgrgid ();
#endif

#ifndef HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

#ifdef HAVE_LCHOWN
# define LCHOWN(FILE, OWNER, GROUP) lchown (FILE, OWNER, GROUP)
#else
# define LCHOWN(FILE, OWNER, GROUP) 1
#endif

char *parse_user_spec ();
void strip_trailing_slashes ();
char *xmalloc ();
char *xrealloc ();

enum Change_status
{
  CH_SUCCEEDED,
  CH_FAILED,
  CH_NO_CHANGE_REQUESTED
};

static int change_dir_owner __P ((const char *dir, uid_t user, gid_t group,
				  struct stat *statp));

/* The name the program was run with. */
char *program_name;

/* If nonzero, and the systems has support for it, change the ownership
   of symbolic links rather than any files they point to.  */
static int change_symlinks;

/* If nonzero, change the ownership of directories recursively. */
static int recurse;

/* If nonzero, force silence (no error messages). */
static int force_silent;

/* If nonzero, describe the files we process. */
static int verbose;

/* If nonzero, describe only owners or groups that change. */
static int changes_only;

/* The name of the user to which ownership of the files is being given. */
static char *username;

/* The name of the group to which ownership of the files is being given. */
static char *groupname;

/* The argument to the --reference option.  Use the owner and group IDs
   of this file.  This file must exist.  */
static char *reference_file;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"recursive", no_argument, 0, 'R'},
  {"changes", no_argument, 0, 'c'},
  {"no-dereference", no_argument, 0, 'h'},
  {"quiet", no_argument, 0, 'f'},
  {"silent", no_argument, 0, 'f'},
  {"reference", required_argument, 0, 12},
  {"verbose", no_argument, 0, 'v'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

/* Tell the user how/if the user and group of FILE have been changed.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, enum Change_status changed)
{
  const char *fmt;
  switch (changed)
    {
    case CH_SUCCEEDED:
      fmt = _("owner of %s changed to ");
      break;
    case CH_FAILED:
      fmt = _("failed to change owner of %s to ");
      break;
    case CH_NO_CHANGE_REQUESTED:
      fmt = _("owner of %s retained as ");
      break;
    default:
      abort ();
    }
  printf (fmt, file);
  if (groupname)
    printf ("%s.%s\n", username, groupname);
  else
    printf ("%s\n", username);
}

/* Change the ownership of FILE to UID USER and GID GROUP.
   If it is a directory and -R is given, recurse.
   Return 0 if successful, 1 if errors occurred. */

static int
change_file_owner (const char *file, uid_t user, gid_t group)
{
  struct stat file_stats;
  uid_t newuser;
  gid_t newgroup;
  int errors = 0;

  if (lstat (file, &file_stats))
    {
      if (force_silent == 0)
	error (0, errno, "%s", file);
      return 1;
    }

  newuser = user == (uid_t) -1 ? file_stats.st_uid : user;
  newgroup = group == (gid_t) -1 ? file_stats.st_gid : group;
  if (newuser != file_stats.st_uid || newgroup != file_stats.st_gid)
    {
      int fail;

      if (change_symlinks)
	fail = LCHOWN (file, newuser, newgroup);
      else
	fail = chown (file, newuser, newgroup);

      if (verbose || (changes_only && !fail))
	describe_change (file, (fail ? CH_FAILED : CH_SUCCEEDED));

      if (fail)
	{
	  if (force_silent == 0)
	    error (0, errno, "%s", file);
	  errors = 1;
	}
    }
  else if (verbose && changes_only == 0)
    {
      describe_change (file, CH_NO_CHANGE_REQUESTED);
    }

  if (recurse && S_ISDIR (file_stats.st_mode))
    errors |= change_dir_owner (file, user, group, &file_stats);
  return errors;
}

/* Recursively change the ownership of the files in directory DIR
   to UID USER and GID GROUP.
   STATP points to the results of lstat on DIR.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_owner (const char *dir, uid_t user, gid_t group, struct stat *statp)
{
  char *name_space, *namep;
  char *path;			/* Full path of each entry to process. */
  unsigned dirlength;		/* Length of `dir' and '\0'. */
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
	error (1, 0, _("virtual memory exhausted"));
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
      errors |= change_file_owner (path, user, group);
    }
  free (path);
  free (name_space);
  return errors;
}

static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
	     program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... OWNER[.[GROUP]] FILE...\n\
  or:  %s [OPTION]... .GROUP FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name, program_name);
      printf (_("\
Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\
\n\
  -c, --changes          be verbose whenever change occurs\n\
  -h, --no-dereference   affect symbolic links instead of any referenced file\n\
                         (available only on systems that can change the\n\
                         ownership of a symlink)\n\
  -f, --silent, --quiet  suppress most error messages\n\
      --reference=RFILE  use the owner and group of RFILE instead of using\n\
                         explicit OWNER.GROUP values\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          explain what is being done\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
\n\
Owner is unchanged if missing.  Group is unchanged if missing, but changed\n\
to login group if implied by a period.  A colon may replace the period.\n\
"));
      puts (_("\nReport bugs to <fileutils-bugs@gnu.ai.mit.edu>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  uid_t user = (uid_t) -1;	/* New uid; -1 if not to be changed. */
  gid_t group = (uid_t) -1;	/* New gid; -1 if not to be changed. */
  int errors = 0;
  int optc;
  char *e;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  recurse = force_silent = verbose = changes_only = 0;

  while ((optc = getopt_long (argc, argv, "Rcfhv", long_options, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 12:
	  reference_file = optarg;
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
	case 'h':
	  change_symlinks = 1;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("chown (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (argc - optind + (reference_file ? 1 : 0) <= 1)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

#ifndef HAVE_LCHOWN
  if (change_symlinks)
    {
      error (1, 0, _("--no-dereference (-h) is not supported on this system"));
    }
#endif

  if (reference_file)
    {
      struct stat ref_stats;

      if (stat (reference_file, &ref_stats))
        error (1, errno, "%s", reference_file);

      user  = ref_stats.st_uid;
      group = ref_stats.st_gid;
    }
  else
    {
      e = parse_user_spec (argv[optind], &user, &group, &username, &groupname);
      if (e)
        error (1, 0, "%s: %s", argv[optind], e);
      if (username == NULL)
        username = "";

      optind++;
    }

  for (; optind < argc; ++optind)
    {
      strip_trailing_slashes (argv[optind]);
      errors |= change_file_owner (argv[optind], user, group);
    }

  exit (errors);
}
