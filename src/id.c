/* id -- print real and effective UIDs and GIDs
   Copyright (C) 1989, 1991 Free Software Foundation.

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

/* Written by Arnold Robbins, arnold@audiofax.com.
   Major rewrite by David MacKenzie, djm@gnu.ai.mit.edu. */

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>

#include "version.h"
#include "system.h"

#ifdef _POSIX_VERSION
#include <limits.h>
#if !defined(NGROUPS_MAX) || NGROUPS_MAX < 1
#undef NGROUPS_MAX
#define NGROUPS_MAX sysconf (_SC_NGROUPS_MAX)
#endif /* !NGROUPS_MAX */

#else /* not _POSIX_VERSION */
struct passwd *getpwuid ();
struct group *getgrgid ();
uid_t getuid ();
gid_t getgid ();
uid_t geteuid ();
gid_t getegid ();
#include <sys/param.h>
#if !defined(NGROUPS_MAX) && defined(NGROUPS)
#define NGROUPS_MAX NGROUPS
#endif /* not NGROUPS_MAX and NGROUPS */
#endif /* not _POSIX_VERSION */

char *xmalloc ();
int getugroups ();
void error ();

static void print_user ();
static void print_group ();
static void print_group_list ();
static void print_full_info ();
static void usage ();

/* The name this program was run with. */
char *program_name;

/* If nonzero, output only the group ID(s). -g */
static int just_group = 0;

/* If nonzero, output user/group name instead of ID number. -n */
static int use_name = 0;

/* If nonzero, output real UID/GID instead of default effective UID/GID. -r */
static int use_real = 0;

/* If nonzero, output only the user ID(s). -u */
static int just_user = 0;

/* If nonzero, output only the supplementary groups. -G */
static int just_group_list = 0;

/* The real and effective IDs of the user to print. */
static uid_t ruid, euid;
static gid_t rgid, egid;

/* The number of errors encountered so far. */
static int problems = 0;

/* If non-zero, display usage information and exit.  */
static int show_help;

/* If non-zero, print the version on standard output and exit.  */
static int show_version;

static struct option const longopts[] =
{
  {"group", no_argument, NULL, 'g'},
  {"groups", no_argument, NULL, 'G'},
  {"help", no_argument, &show_help, 1},
  {"name", no_argument, NULL, 'n'},
  {"real", no_argument, NULL, 'r'},
  {"user", no_argument, NULL, 'u'},
  {"version", no_argument, &show_version, 1},
  {NULL, 0, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  int optc;

  program_name = argv[0];

  while ((optc = getopt_long (argc, argv, "gnruG", longopts, (int *) 0))
	 != EOF)
    {
      switch (optc)
	{
	case 0:
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
	default:
	  usage (1);
	}
    }

  if (show_version)
    {
      printf ("%s\n", version_string);
      exit (0);
    }

  if (show_help)
    usage (0);

  if (just_user + just_group + just_group_list > 1)
    error (1, 0, "cannot print only user and only group");

  if (just_user + just_group + just_group_list == 0 && (use_real || use_name))
    error (1, 0, "cannot print only names or real IDs in default format");

  if (argc - optind > 1)
    usage (1);

  if (argc - optind == 1)
    {
      struct passwd *pwd = getpwnam (argv[optind]);
      if (pwd == NULL)
	error (1, 0, "%s: No such user", argv[optind]);
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
print_user (uid)
     int uid;
{
  struct passwd *pwd = NULL;

  if (use_name)
    {
      pwd = getpwuid (uid);
      if (pwd == NULL)
	problems++;
    }

  if (pwd == NULL)
    printf ("%u", (unsigned) uid);
  else
    printf ("%s", pwd->pw_name);
}

/* Print the name or value of group ID GID. */

static void
print_group (gid)
     int gid;
{
  struct group *grp = NULL;

  if (use_name)
    {
      grp = getgrgid (gid);
      if (grp == NULL)
	problems++;
    }

  if (grp == NULL)
    printf ("%u", (unsigned) gid);
  else
    printf ("%s", grp->gr_name);
}

/* Print all of the distinct groups the user is in . */

static void
print_group_list (username)
     char *username;
{
  print_group (rgid);
  if (egid != rgid)
    {
      putchar (' ');
      print_group (egid);
    }

#ifdef NGROUPS_MAX
  {
    int ngroups;
    GETGROUPS_T *groups;
    register int i;

    groups = (GETGROUPS_T *) xmalloc (NGROUPS_MAX * sizeof (GETGROUPS_T));
    if (username == 0)
      ngroups = getgroups (NGROUPS_MAX, groups);
    else
      ngroups = getugroups (NGROUPS_MAX, groups, username);
    if (ngroups < 0)
      {
	error (0, errno, "cannot get supplemental group list");
	problems++;
	free (groups);
	return;
      }

    for (i = 0; i < ngroups; i++)
      if (groups[i] != rgid && groups[i] != egid)
	{
	  putchar (' ');
	  print_group (groups[i]);
	}
    free (groups);
  }
#endif
}

/* Print all of the info about the user's user and group IDs. */

static void
print_full_info (username)
     char *username;
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

#ifdef NGROUPS_MAX
  {
    int ngroups;
    GETGROUPS_T *groups;
    register int i;

    groups = (GETGROUPS_T *) xmalloc (NGROUPS_MAX * sizeof (GETGROUPS_T));
    if (username == 0)
      ngroups = getgroups (NGROUPS_MAX, groups);
    else
      ngroups = getugroups (NGROUPS_MAX, groups, username);
    if (ngroups < 0)
      {
	error (0, errno, "cannot get supplemental group list");
	problems++;
	free (groups);
	return;
      }

    if (ngroups > 0)
      fputs (" groups=", stdout);
    for (i = 0; i < ngroups; i++)
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
#endif
}

static void
usage (status)
     int status;
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n",
	     program_name);
  else
    {
      printf ("Usage: %s [OPTION]... [USERNAME]\n", program_name);
      printf ("\
\n\
  -g, --group     print only the group ID\n\
  -n, --name      print a name instead of a number, for -ugG\n\
  -r, --real      print the real ID instead of effective ID, for -ugG\n\
  -u, --user      print only the user ID\n\
  -G, --groups    print only the supplementary groups\n\
      --help      display this help and exit\n\
      --version   output version information and exit\n\
\n\
Without any OPTION, print some useful set of identified information.\n\
");
    }
  exit (status);
}
