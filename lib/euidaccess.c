/* euidaccess -- check if effective user id can access file
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.

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

/* Written by David MacKenzie and Torbjorn Granlund. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef S_IEXEC
#ifndef S_IXUSR
#define S_IXUSR S_IEXEC
#endif
#ifndef S_IXGRP
#define S_IXGRP (S_IEXEC >> 3)
#endif
#ifndef S_IXOTH
#define S_IXOTH (S_IEXEC >> 6)
#endif
#endif /* S_IEXEC */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _POSIX_VERSION
#include <limits.h>
#if !defined(NGROUPS_MAX) || NGROUPS_MAX < 1
#undef NGROUPS_MAX
#define NGROUPS_MAX sysconf (_SC_NGROUPS_MAX)
#endif /* NGROUPS_MAX */

#else /* not _POSIX_VERSION */
uid_t getuid ();
gid_t getgid ();
uid_t geteuid ();
gid_t getegid ();
#include <sys/param.h>
#if !defined(NGROUPS_MAX) && defined(NGROUPS)
#define NGROUPS_MAX NGROUPS
#endif /* not NGROUPS_MAX and NGROUPS */
#endif /* not POSIX_VERSION */

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if defined(EACCES) && !defined(EACCESS)
#define EACCESS EACCES
#endif

#ifndef F_OK
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

/* The user's real user id. */
static uid_t uid;

/* The user's real group id. */
static gid_t gid;

/* The user's effective user id. */
static uid_t euid;

/* The user's effective group id. */
static gid_t egid;

int group_member ();

/* Nonzero if UID, GID, EUID, and EGID have valid values. */
static int have_ids = 0;

/* Like euidaccess, except that a pointer to a filled-in stat structure
   describing the file is provided instead of a filename.
   Because this function is almost guaranteed to fail on systems that
   use ACLs, a third argument *PATH may be used.  If it is non-NULL,
   it is assumed to be the name of the file corresponding to STATP.
   Then, if the user is not running set-uid or set-gid, use access
   instead of attempting a manual and non-portable comparison.  */

int
eaccess_stat (statp, mode, path)
     const struct stat *statp;
     int mode;
     const char *path;
{
  int granted;

  mode &= (X_OK | W_OK | R_OK);	/* Clear any bogus bits. */

  if (mode == F_OK)
    return 0;			/* The file exists. */

  if (have_ids == 0)
    {
      have_ids = 1;
      uid = getuid ();
      gid = getgid ();
      euid = geteuid ();
      egid = getegid ();
    }

  if (path && uid == euid && gid == egid)
    {
      return access (path, mode);
    }

  /* The super-user can read and write any file, and execute any file
     that anyone can execute. */
  if (euid == 0 && ((mode & X_OK) == 0
		    || (statp->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))))
    return 0;

  if (euid == statp->st_uid)
    granted = (unsigned) (statp->st_mode & (mode << 6)) >> 6;
  else if (egid == statp->st_gid
#ifdef HAVE_GETGROUPS
	   || group_member (statp->st_gid)
#endif
	   )
    granted = (unsigned) (statp->st_mode & (mode << 3)) >> 3;
  else
    granted = (statp->st_mode & mode);
  if (granted == mode)
    return 0;
  errno = EACCESS;
  return -1;
}

/* Return 0 if the user has permission of type MODE on file PATH;
   otherwise, return -1 and set `errno' to EACCESS.
   Like access, except that it uses the effective user and group
   id's instead of the real ones, and it does not check for read-only
   filesystem, text busy, etc. */

int
euidaccess (path, mode)
     const char *path;
     int mode;
{
  struct stat stats;

  if (have_ids == 0)
    {
      have_ids = 1;
      uid = getuid ();
      gid = getgid ();
      euid = geteuid ();
      egid = getegid ();
    }

  if (uid == euid && gid == egid)
    {
      return access (path, mode);
    }

  if (stat (path, &stats))
    return -1;

  return eaccess_stat (&stats, mode, path);
}

#ifdef TEST
#include <stdio.h>
#include <errno.h>

void error ();

char *program_name;

int
main (argc, argv)
     int argc;
     char **argv;
{
  char *file;
  int mode;
  int err;

  program_name = argv[0];
  if (argc < 3)
    abort ();
  file = argv[1];
  mode = atoi (argv[2]);

  err = euidaccess (file, mode);
  printf ("%d\n", err);
  if (err != 0)
    error (0, errno, "%s", file);
  exit (0);
}
#endif
