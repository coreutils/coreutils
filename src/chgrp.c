/* chgrp -- change group ownership of files
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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>. */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <grp.h>
#include <getopt.h>

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef UINT_MAX
# define UINT_MAX ((unsigned int) ~(unsigned int) 0)
#endif

#ifndef INT_MAX
# define INT_MAX ((int) (UINT_MAX >> 1))
#endif

#include "system.h"
#include "xstrtoul.h"
#include "error.h"

/* MAXUID may come from limits.h *or* sys/params.h (via system.h) above. */
#ifndef MAXUID
# define MAXUID INT_MAX
#endif

#ifndef _POSIX_VERSION
struct group *getgrnam ();
#endif

#ifndef HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#ifdef HAVE_LCHOWN
# define LCHOWN(FILE, OWNER, GROUP) lchown (FILE, OWNER, GROUP)
#else
# define LCHOWN(FILE, OWNER, GROUP) 1
#endif

char *group_member ();
char *savedir ();
char *xmalloc ();
char *xrealloc ();

static int change_dir_group __P ((const char *dir, int group,
				  const struct stat *statp));

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

/* The name of the group to which ownership of the files is being given. */
static const char *groupname;

/* If nonzero, display usage information and exit.  */
static int show_help;

/* If nonzero, print the version on standard output and exit.  */
static int show_version;

static struct option const long_options[] =
{
  {"recursive", no_argument, 0, 'R'},
  {"changes", no_argument, 0, 'c'},
  {"no-dereference", no_argument, 0, 'h'},
  {"silent", no_argument, 0, 'f'},
  {"quiet", no_argument, 0, 'f'},
  {"verbose", no_argument, 0, 'v'},
  {"help", no_argument, &show_help, 1},
  {"version", no_argument, &show_version, 1},
  {0, 0, 0, 0}
};

/* Tell the user the group name to which ownership of FILE
   has been given; if CHANGED is zero, FILE was that group already. */

static void
describe_change (const char *file, int changed)
{
  if (changed)
    printf (_("group of %s changed to %s\n"), file, groupname);
  else
    printf (_("group of %s retained as %s\n"), file, groupname);
}

/* Set *G according to NAME. */

static void
parse_group (const char *name, int *g)
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

      if (tmp_long > INT_MAX)
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
change_file_group (const char *file, int group)
{
  struct stat file_stats;
  int errors = 0;

  if (lstat (file, &file_stats))
    {
      if (force_silent == 0)
	error (0, errno, "%s", file);
      return 1;
    }

  if (group != file_stats.st_gid)
    {
      int fail;

      if (verbose)
	describe_change (file, 1);

      if (change_symlinks)
	fail = LCHOWN (file, (uid_t) -1, group);
      else
	fail = chown (file, (uid_t) -1, group);

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
  else if (verbose && changes_only == 0)
    {
      describe_change (file, 0);
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
change_dir_group (const char *dir, int group, const struct stat *statp)
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
      errors |= change_file_group (path, group);
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
      printf (_("Usage: %s [OPTION]... GROUP FILE...\n"), program_name);
      printf (_("\
Change the group membership of each FILE to GROUP.\n\
\n\
  -c, --changes           like verbose but report only when a change is made\n\
  -h, --no-dereference    affect symbolic links instead of any referenced file\n\
                          (available only on systems with lchown system call)\n\
  -f, --silent, --quiet   suppress most error messages\n\
  -R, --recursive         change files and directories recursively\n\
  -v, --verbose           output a diagnostic for every file processed\n\
      --help              display this help and exit\n\
      --version           output version information and exit\n\
"));
      puts (_("\nReport bugs to <fileutils-bugs@gnu.ai.mit.edu>."));
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int group;
  int errors = 0;
  int optc;

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
      printf ("chgrp (%s) %s\n", GNU_PACKAGE, VERSION);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (argc - optind <= 1)
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

  parse_group (argv[optind++], &group);

  for (; optind < argc; ++optind)
    errors |= change_file_group (argv[optind], group);

  exit (errors);
}
