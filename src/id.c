/* id -- print real and effective UIDs and GIDs
   Copyright (C) 1989-2003 Free Software Foundation, Inc.

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

/* Written by Arnold Robbins.
   Major rewrite by David MacKenzie, djm@gnu.ai.mit.edu. */

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "closeout.h"

/* The official name of this program (e.g., no `g' prefix).  */
#define PROGRAM_NAME "id"

#define AUTHORS N_ ("Arnold Robbins and David MacKenzie")

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
struct group *getgrgid ();
uid_t getuid ();
gid_t getgid ();
uid_t geteuid ();
gid_t getegid ();
#endif /* not _POSIX_VERSION */

int getugroups ();

static void print_user (uid_t uid);
static void print_group (gid_t gid);
static void print_group_list (const char *username);
static void print_full_info (const char *username);

/* The name this program was run with. */
char *program_name;

/* If nonzero, output user/group name instead of ID number. -n */
static int use_name = 0;

/* The real and effective IDs of the user to print. */
static uid_t ruid, euid;
static gid_t rgid, egid;

/* The number of errors encountered so far. */
static int problems = 0;

static struct option const longopts[] =
{
  {"group", no_argument, NULL, 'g'},
  {"groups", no_argument, NULL, 'G'},
  {"name", no_argument, NULL, 'n'},
  {"real", no_argument, NULL, 'r'},
  {"user", no_argument, NULL, 'u'},
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
      printf (_("Usage: %s [OPTION]... [USERNAME]\n"), program_name);
      fputs (_("\
Print information for USERNAME, or the current user.\n\
\n\
  -a              ignore, for compatibility with other versions\n\
  -g, --group     print only the effective group ID\n\
  -G, --groups    print all group IDs\n\
  -n, --name      print a name instead of a number, for -ugG\n\
  -r, --real      print the real ID instead of the effective ID, with -ugG\n\
  -u, --user      print only the effective user ID\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
Without any OPTION, print some useful set of identified information.\n\
"), stdout);
      printf (_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
    }
  exit (status);
}

int
main (int argc, char **argv)
{
  int optc;

  /* If nonzero, output the list of all group IDs. -G */
  int just_group_list = 0;
  /* If nonzero, output only the group ID(s). -g */
  int just_group = 0;
  /* If nonzero, output real UID/GID instead of default effective UID/GID. -r */
  int use_real = 0;
  /* If nonzero, output only the user ID(s). -u */
  int just_user = 0;

  program_name = argv[0];
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "agnruG", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 0:
	  break;
	case 'a':
	  /* Ignore -a, for compatibility with SVR4.  */
	  break;
	case 'g':
	  just_group = 1;
	  break;
	case 'n':
	  use_name = 1;
	  break;
	case 'r':
	  use_real = 1;
	  break;
	case 'u':
	  just_user = 1;
	  break;
	case 'G':
	  just_group_list = 1;
	  break;
	case_GETOPT_HELP_CHAR;
	case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);
	default:
	  usage (EXIT_FAILURE);
	}
    }

  if (just_user + just_group + just_group_list > 1)
    error (EXIT_FAILURE, 0, _("cannot print only user and only group"));

  if (just_user + just_group + just_group_list == 0 && (use_real || use_name))
    error (EXIT_FAILURE, 0,
	   _("cannot print only names or real IDs in default format"));

  if (argc - optind > 1)
    usage (EXIT_FAILURE);

  if (argc - optind == 1)
    {
      struct passwd *pwd = getpwnam (argv[optind]);
      if (pwd == NULL)
	error (EXIT_FAILURE, 0, _("%s: No such user"), argv[optind]);
      ruid = euid = pwd->pw_uid;
      rgid = egid = pwd->pw_gid;
    }
  else
    {
      euid = geteuid ();
      ruid = getuid ();
      egid = getegid ();
      rgid = getgid ();
    }

  if (just_user)
    print_user (use_real ? ruid : euid);
  else if (just_group)
    print_group (use_real ? rgid : egid);
  else if (just_group_list)
    print_group_list (argv[optind]);
  else
    print_full_info (argv[optind]);
  putchar ('\n');

  exit (problems != 0);
}

/* Print the name or value of user ID UID. */

static void
print_user (uid_t uid)
{
  struct passwd *pwd = NULL;

  if (use_name)
    {
      pwd = getpwuid (uid);
      if (pwd == NULL)
	{
	  error (0, 0, _("cannot find name for user ID %u"), uid);
	  problems++;
	}
    }

  if (pwd == NULL)
    printf ("%u", (unsigned) uid);
  else
    printf ("%s", pwd->pw_name);
}

/* Print the name or value of group ID GID. */

static void
print_group (gid_t gid)
{
  struct group *grp = NULL;

  if (use_name)
    {
      grp = getgrgid (gid);
      if (grp == NULL)
	{
	  error (0, 0, _("cannot find name for group ID %u"), gid);
	  problems++;
	}
    }

  if (grp == NULL)
    printf ("%u", (unsigned) gid);
  else
    printf ("%s", grp->gr_name);
}

#if HAVE_GETGROUPS

/* FIXME: document */

static int
xgetgroups (const char *username, gid_t gid, int *n_groups,
	    GETGROUPS_T **groups)
{
  int max_n_groups;
  int ng;
  GETGROUPS_T *g;
  int fail = 0;

  if (username == 0)
    max_n_groups = getgroups (0, NULL);
  else
    max_n_groups = getugroups (0, NULL, username, gid);

  /* Add 1 just in case max_n_groups is zero.  */
  g = (GETGROUPS_T *) xmalloc (max_n_groups * sizeof (GETGROUPS_T) + 1);
  if (username == 0)
    ng = getgroups (max_n_groups, g);
  else
    ng = getugroups (max_n_groups, g, username, gid);

  if (ng < 0)
    {
      error (0, errno, _("cannot get supplemental group list"));
      ++fail;
      free (groups);
    }
  if (!fail)
    {
      *n_groups = ng;
      *groups = g;
    }
  return fail;
}

#endif /* HAVE_GETGROUPS */

/* Print all of the distinct groups the user is in. */

static void
print_group_list (const char *username)
{
  struct passwd *pwd;

  pwd = getpwuid (ruid);
  if (pwd == NULL)
    problems++;

  print_group (rgid);
  if (egid != rgid)
    {
      putchar (' ');
      print_group (egid);
    }

#if HAVE_GETGROUPS
  {
    int n_groups;
    GETGROUPS_T *groups;
    register int i;

    if (xgetgroups (username, (pwd ? pwd->pw_gid : (gid_t) -1),
		    &n_groups, &groups))
      {
	++problems;
	return;
      }

    for (i = 0; i < n_groups; i++)
      if (groups[i] != rgid && groups[i] != egid)
	{
	  putchar (' ');
	  print_group (groups[i]);
	}
    free (groups);
  }
#endif /* HAVE_GETGROUPS */
}

/* Print all of the info about the user's user and group IDs. */

static void
print_full_info (const char *username)
{
  struct passwd *pwd;
  struct group *grp;

  printf ("uid=%u", (unsigned) ruid);
  pwd = getpwuid (ruid);
  if (pwd == NULL)
    problems++;
  else
    printf ("(%s)", pwd->pw_name);

  printf (" gid=%u", (unsigned) rgid);
  grp = getgrgid (rgid);
  if (grp == NULL)
    problems++;
  else
    printf ("(%s)", grp->gr_name);

  if (euid != ruid)
    {
      printf (" euid=%u", (unsigned) euid);
      pwd = getpwuid (euid);
      if (pwd == NULL)
	problems++;
      else
	printf ("(%s)", pwd->pw_name);
    }

  if (egid != rgid)
    {
      printf (" egid=%u", (unsigned) egid);
      grp = getgrgid (egid);
      if (grp == NULL)
	problems++;
      else
	printf ("(%s)", grp->gr_name);
    }

#if HAVE_GETGROUPS
  {
    int n_groups;
    GETGROUPS_T *groups;
    register int i;

    if (xgetgroups (username, (pwd ? pwd->pw_gid : (gid_t) -1),
		    &n_groups, &groups))
      {
	++problems;
	return;
      }

    if (n_groups > 0)
      fputs (_(" groups="), stdout);
    for (i = 0; i < n_groups; i++)
      {
	if (i > 0)
	  putchar (',');
	printf ("%u", (unsigned) groups[i]);
	grp = getgrgid (groups[i]);
	if (grp == NULL)
	  problems++;
	else
	  printf ("(%s)", grp->gr_name);
      }
    free (groups);
  }
#endif /* HAVE_GETGROUPS */
}
