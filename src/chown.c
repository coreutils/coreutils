/* chown -- change user and group ownership of files
   Copyright (C) 89, 90, 91, 1995-2000 Free Software Foundation, Inc.

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
#include "lchown.h"
#include "quote.h"
#include "savedir.h"

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
void strip_trailing_slashes ();

enum Change_status
{
  CH_NOT_APPLIED,
  CH_SUCCEEDED,
  CH_FAILED,
  CH_NO_CHANGE_REQUESTED
};

enum Verbosity
{
  /* Print a message for each file that is processed.  */
  V_high,

  /* Print a message for each file whose attributes we change.  */
  V_changes_only,

  /* Do not be verbose.  This is the default. */
  V_off
};

static int change_dir_owner PARAMS ((const char *dir, uid_t user, gid_t group,
				     uid_t old_user, gid_t old_group,
				     const struct stat *statp));

/* The name the program was run with. */
char *program_name;

/* If nonzero, and the systems has support for it, change the ownership
   of symbolic links rather than any files they point to.  */
static int change_symlinks = 1;

/* If nonzero, change the ownership of directories recursively. */
static int recurse;

/* If nonzero, force silence (no error messages). */
static int force_silent;

/* Level of verbosity.  */
static enum Verbosity verbosity = V_off;

/* The name of the user to which ownership of the files is being given. */
static char *username;

/* The name of the group to which ownership of the files is being given. */
static const char *groupname;

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

/* Tell the user how/if the user and group of FILE have been changed.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, enum Change_status changed)
{
  const char *fmt;

  if (changed == CH_NOT_APPLIED)
    {
      printf (_("neither symbolic link %s nor referent has been changed\n"),
	      quote (file));
      return;
    }

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

/* Change the ownership of FILE to UID USER and GID GROUP
   provided it presently has UID OLDUSER and GID OLDGROUP.
   If it is a directory and -R is given, recurse.
   Return 0 if successful, 1 if errors occurred. */

static int
change_file_owner (int cmdline_arg, const char *file, uid_t user, gid_t group,
		   uid_t old_user, gid_t old_group)
{
  struct stat file_stats;
  uid_t newuser;
  gid_t newgroup;
  int errors = 0;

  if (lstat (file, &file_stats))
    {
      if (force_silent == 0)
	error (0, errno, _("getting attributes of %s"), quote (file));
      return 1;
    }

  if ((old_user == (uid_t) -1  || file_stats.st_uid == old_user) &&
      (old_group == (gid_t) -1 || file_stats.st_gid == old_group))
    {
      newuser = user == (uid_t) -1 ? file_stats.st_uid : user;
      newgroup = group == (gid_t) -1 ? file_stats.st_gid : group;
      if (newuser != file_stats.st_uid || newgroup != file_stats.st_gid)
	{
	  int fail;
	  int symlink_changed = 1;
	  int saved_errno;

	  if (S_ISLNK (file_stats.st_mode) && change_symlinks)
	    {
	      fail = lchown (file, newuser, newgroup);

	      /* Ignore the failure if it's due to lack of support (ENOSYS)
		 and this is not a command line argument.  */
	      if (!cmdline_arg && fail && errno == ENOSYS)
		{
		  fail = 0;
		  symlink_changed = 0;
		}
	    }
	  else
	    {
	      fail = chown (file, newuser, newgroup);
	    }
	  saved_errno = errno;

	  if (verbosity == V_high || (verbosity == V_changes_only && !fail))
	    {
	      enum Change_status ch_status = (! symlink_changed
					      ? CH_NOT_APPLIED
					      : (fail
						 ? CH_FAILED : CH_SUCCEEDED));
	      describe_change (file, ch_status);
	    }

	  if (fail)
	    {
	      if (force_silent == 0)
		error (0, saved_errno, _("changing ownership of %s"),
		       quote (file));
	      errors = 1;
	    }
	  else
	    {
	      /* The change succeeded.  On some systems, the chown function
		 resets the `special' permission bits.  When run by a
		 `privileged' user, this program must ensure that at least
		 the set-uid and set-group ones are still set.  */
	      if (file_stats.st_mode & ~S_IRWXUGO
		  /* If this is a symlink and we changed *it*, then skip it.  */
		  && ! (S_ISLNK (file_stats.st_mode) && change_symlinks))
		{
		  if (chmod (file, file_stats.st_mode))
		    {
		      error (0, saved_errno,
			     _("unable to restore permissions of %s"),
			     quote (file));
		      fail = 1;
		    }
		}
	    }
	}
      else if (verbosity == V_high)
	{
	  describe_change (file, CH_NO_CHANGE_REQUESTED);
	}
    }

  if (recurse && S_ISDIR (file_stats.st_mode))
    errors |= change_dir_owner (file, user, group,
				old_user, old_group, &file_stats);
  return errors;
}

/* Recursively change the ownership of the files in directory DIR
   to UID USER and GID GROUP.
   STATP points to the results of lstat on DIR.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_owner (const char *dir, uid_t user, gid_t group,
		  uid_t old_user, gid_t old_group,
		  const struct stat *statp)
{
  char *name_space, *namep;
  char *path;			/* Full path of each entry to process. */
  unsigned dirlength;		/* Length of `dir' and '\0'. */
  unsigned filelength;		/* Length of each pathname to process. */
  unsigned pathlength;		/* Bytes allocated for `path'. */
  int errors = 0;

  name_space = savedir (dir, statp->st_size);
  if (name_space == NULL)
    {
      if (force_silent == 0)
	error (0, errno, "%s", quote (dir));
      return 1;
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
      errors |= change_file_owner (0, path, user, group, old_user, old_group);
    }
  free (path);
  free (name_space);
  return errors;
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
Usage: %s [OPTION]... OWNER[:[GROUP]] FILE...\n\
  or:  %s [OPTION]... :GROUP FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name, program_name);
      printf (_("\
Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\
\n\
  -c, --changes          like verbose but report only when a change is made\n\
      --dereference      affect the referent of each symbolic link, rather\n\
                         than the symbolic link itself\n\
  -h, --no-dereference   affect symbolic links instead of any referenced file\n\
                         (available only on systems that can change the\n\
                         ownership of a symlink)\n\
      --from=CURRENT_OWNER:CURRENT_GROUP\n\
                         change the owner and/or group of each file only if\n\
                         its current owner and/or group match those specified\n\
                         here.  Either may be omitted, in which case a match\n\
                         is not required for the omitted attribute.\n\
  -f, --silent, --quiet  suppress most error messages\n\
      --reference=RFILE  use RFILE's owner and group rather than\n\
                         the specified OWNER:GROUP values\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          output a diagnostic for every file processed\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
\n\
Owner is unchanged if missing.  Group is unchanged if missing, but changed\n\
to login group if implied by a `:'.  OWNER and GROUP may be numeric as well\n\
as symbolic.\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  uid_t user = (uid_t) -1;	/* New uid; -1 if not to be changed. */
  gid_t group = (uid_t) -1;	/* New gid; -1 if not to be changed. */
  uid_t old_user = (uid_t) -1;	/* Old uid; -1 if unrestricted. */
  gid_t old_group = (uid_t) -1;	/* Old gid; -1 if unrestricted. */

  int errors = 0;
  int optc;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  recurse = force_silent = 0;

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
	  change_symlinks = 0;
	  break;
	case FROM_OPTION:
	  {
	    char *u_dummy, *g_dummy;
	    const char *e = parse_user_spec (argv[optind],
					     &old_user, &old_group,
					     &u_dummy, &g_dummy);
	    if (e)
	      error (1, 0, "%s: %s", quote (argv[optind]), e);
	    break;
	  }
	case 'R':
	  recurse = 1;
	  break;
	case 'c':
	  verbosity = V_changes_only;
	  break;
	case 'f':
	  force_silent = 1;
	  break;
	case 'h':
	  change_symlinks = 1;
	  break;
	case 'v':
	  verbosity = V_high;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (1);
	}
    }

  if (argc - optind + (reference_file ? 1 : 0) <= 1)
    {
      error (0, 0, _("too few arguments"));
      usage (1);
    }

  if (reference_file)
    {
      struct stat ref_stats;

      if (stat (reference_file, &ref_stats))
	error (1, errno, _("getting attributes of %s"), quote (reference_file));

      user  = ref_stats.st_uid;
      group = ref_stats.st_gid;
    }
  else
    {
      const char *e = parse_user_spec (argv[optind], &user, &group,
				       &username, &groupname);
      if (e)
        error (1, 0, "%s: %s", argv[optind], e);
      if (username == NULL)
        username = "";

      optind++;
    }

  for (; optind < argc; ++optind)
    {
      strip_trailing_slashes (argv[optind]);
      errors |= change_file_owner (1, argv[optind], user, group,
				   old_user, old_group);
    }

  exit (errors);
}
