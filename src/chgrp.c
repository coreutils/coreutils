/* chgrp -- change group ownership of files
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
#include "savedir.h"
#include "xstrtol.h"

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

int lstat ();

static int change_dir_group PARAMS ((const char *dir, gid_t group,
				     const struct stat *statp));

/* The name the program was run with. */
char *program_name;

/* If nonzero, and the systems has support for it, change the ownership
   of symbolic links rather than any files they point to.  */
static int change_symlinks = 1;

/* When change_symlinks is set, this should be set to `lstat', otherwise,
   it should be `stat'.  */
static int (*xstat) ();

/* If nonzero, change the ownership of directories recursively. */
static int recurse;

/* If nonzero, force silence (no error messages). */
static int force_silent;

/* Level of verbosity.  */
static enum Verbosity verbosity = V_off;

/* The name of the group to which ownership of the files is being given. */
static const char *groupname;

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

/* Tell the user how/if the group of FILE has been changed.
   CHANGED describes what (if anything) has happened. */

static void
describe_change (const char *file, enum Change_status changed)
{
  const char *fmt;

  if (changed == CH_NOT_APPLIED)
    {
      printf (_("neither symbolic link %s nor referent has been changed\n"),
	      file);
      return;
    }

  switch (changed)
    {
    case CH_SUCCEEDED:
      fmt = _("group of %s changed to %s\n");
      break;
    case CH_FAILED:
      fmt = _("failed to change group of %s to %s\n");
      break;
    case CH_NO_CHANGE_REQUESTED:
      fmt = _("group of %s retained as %s\n");
      break;
    default:
      abort ();
    }
  printf (fmt, file, groupname);
}

/* Set *G according to NAME. */

static void
parse_group (const char *name, gid_t *g)
{
  struct group *grp;

  groupname = name;
  if (*name == '\0')
    error (1, 0, _("can not change to null group"));

  grp = getgrnam (name);
  if (grp == NULL)
    {
      strtol_error s_err;
      unsigned long int tmp_long;

      if (!ISDIGIT (*name))
	error (1, 0, _("invalid group name `%s'"), name);

      s_err = xstrtoul (name, NULL, 0, &tmp_long, NULL);
      if (s_err != LONGINT_OK)
	STRTOL_FATAL_ERROR (name, _("group number"), s_err);

      if (tmp_long > MAXGID)
	error (1, 0, _("invalid group number `%s'"), name);

      *g = tmp_long;
    }
  else
    *g = grp->gr_gid;
  endgrent ();		/* Save a file descriptor. */
}

/* Change the ownership of FILE to GID GROUP.
   If it is a directory and -R is given, recurse.
   Return 0 if successful, 1 if errors occurred. */

static int
change_file_group (int cmdline_arg, const char *file, gid_t group)
{
  struct stat file_stats;
  int errors = 0;

  if ((*xstat) (file, &file_stats))
    {
      if (force_silent == 0)
	error (0, errno, "%s", file);
      return 1;
    }

  if (group != file_stats.st_gid)
    {
      int fail;
      int symlink_changed = 1;

      if (S_ISLNK (file_stats.st_mode) && change_symlinks)
	{
	  fail = lchown (file, (uid_t) -1, group);

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
	  fail = chown (file, (uid_t) -1, group);
	}

      if (verbosity == V_high || (verbosity == V_changes_only && !fail))
	{
	  enum Change_status ch_status = (! symlink_changed ? CH_NOT_APPLIED
					  : (fail ? CH_FAILED : CH_SUCCEEDED));
	  describe_change (file, ch_status);
	}

      if (fail)
	{
	  errors = 1;
	  if (force_silent == 0)
	    {
	      /* Give a more specific message.  Some systems set errno
		 to EPERM for both `inaccessible file' and `user not a member
		 of the specified group' errors.  */
	      if (errno == EPERM && !group_member (group))
		{
		  error (0, errno, _("you are not a member of group `%s'"),
			 groupname);
		}
	      else if (errno == EINVAL && group > MAXUID)
		{
		  error (0, 0, _("%s: invalid group number"), groupname);
		}
	      else
		{
		  error (0, errno, "%s", file);
		}
	    }
	}
    }
  else if (verbosity == V_high)
    {
      describe_change (file, CH_NO_CHANGE_REQUESTED);
    }

  if (recurse && S_ISDIR (file_stats.st_mode))
    errors |= change_dir_group (file, group, &file_stats);

  return errors;
}

/* Recursively change the ownership of the files in directory DIR
   to GID GROUP.
   STATP points to the results of lstat on DIR.
   Return 0 if successful, 1 if errors occurred. */

static int
change_dir_group (const char *dir, gid_t group, const struct stat *statp)
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
      errors |= change_file_group (0, path, group);
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
Usage: %s [OPTION]... GROUP FILE...\n\
  or:  %s [OPTION]... --reference=RFILE FILE...\n\
"),
	      program_name, program_name);
      printf (_("\
Change the group membership of each FILE to GROUP.\n\
\n\
  -c, --changes          like verbose but report only when a change is made\n\
      --dereference      affect the referent of each symbolic link, rather\n\
                         than the symbolic link itself\n\
  -h, --no-dereference   affect symbolic links instead of any referenced file\n\
                         (available only on systems that can change the\n\
                         ownership of a symlink)\n\
  -f, --silent, --quiet  suppress most error messages\n\
      --reference=RFILE  use RFILE's group rather than the specified\n\
                         GROUP value\n\
  -R, --recursive        operate on files and directories recursively\n\
  -v, --verbose          output a diagnostic for every file processed\n\
      --help             display this help and exit\n\
      --version          output version information and exit\n\
"));
      puts (_("\nReport bugs to <bug-fileutils@gnu.org>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  gid_t group;
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

  if (change_symlinks)
    xstat = lstat;
  else
    xstat = stat;

  if (reference_file)
    {
      struct stat ref_stats;
      if (stat (reference_file, &ref_stats))
        error (1, errno, "%s", reference_file);

      group = ref_stats.st_gid;
    }
  else
    parse_group (argv[optind++], &group);

  for (; optind < argc; ++optind)
    errors |= change_file_group (1, argv[optind], group);

  exit (errors);
}
