/* idcache.c -- map user and group IDs, cached for speed
   Copyright (C) 1985, 1988, 1989, 1990, 1997 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
#else
# include <strings.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
struct passwd *getpwnam ();
struct group *getgrgid ();
struct group *getgrnam ();
#endif

char *xmalloc ();
char *xstrdup ();

struct userid
{
  union
    {
      uid_t u;
      gid_t g;
    } id;
  char *name;
  struct userid *next;
};

/* The members of this list have already been looked up.
   If a name is NULL, the corresponding id is not in the password file.  */
static struct userid *user_alist;

#ifdef NOT_USED
/* The members of this list are names not in the local passwd file;
   their names are always not NULL, and their ids are irrelevant.  */
static struct userid *nouser_alist;
#endif /* NOT_USED */

/* Translate UID to a login name, with cache.
   If UID cannot be resolved, return NULL.
   Cache lookup failures, too.  */

char *
getuser (uid)
     uid_t uid;
{
  register struct userid *tail;
  struct passwd *pwent;

  for (tail = user_alist; tail; tail = tail->next)
    if (tail->id.u == uid)
      return tail->name;

  pwent = getpwuid (uid);
  tail = (struct userid *) xmalloc (sizeof (struct userid));
  tail->id.u = uid;
  tail->name = (pwent ? xstrdup (pwent->pw_name) : NULL);

  /* Add to the head of the list, so most recently added is first.  */
  tail->next = user_alist;
  user_alist = tail;
  return tail->name;
}

#ifdef NOT_USED

/* Translate USER to a UID, with cache.
   Return NULL if there is no such user.
   (We also cache which user names have no passwd entry,
   so we don't keep looking them up.)  */

uid_t *
getuidbyname (user)
     const char *user;
{
  register struct userid *tail;
  struct passwd *pwent;

  for (tail = user_alist; tail; tail = tail->next)
    /* Avoid a function call for the most common case.  */
    if (tail->name && *tail->name == *user && !strcmp (tail->name, user))
      return &tail->id.u;

  for (tail = nouser_alist; tail; tail = tail->next)
    /* Avoid a function call for the most common case.  */
    if (*tail->name == *user && !strcmp (tail->name, user))
      return 0;

  pwent = getpwnam (user);

  tail = (struct userid *) xmalloc (sizeof (struct userid));
  tail->name = xstrdup (user);

  /* Add to the head of the list, so most recently added is first.  */
  if (pwent)
    {
      tail->id.u = pwent->pw_uid;
      tail->next = user_alist;
      user_alist = tail;
      return &tail->id.u;
    }

  tail->next = nouser_alist;
  nouser_alist = tail;
  return 0;
}

#endif /* NOT_USED */

/* Use the same struct as for userids.  */
static struct userid *group_alist;
#ifdef NOT_USED
static struct userid *nogroup_alist;
#endif

/* Translate GID to a group name, with cache.
   Return NULL if the group has no name.  */

char *
getgroup (gid)
     gid_t gid;
{
  register struct userid *tail;
  struct group *grent;

  for (tail = group_alist; tail; tail = tail->next)
    if (tail->id.g == gid)
      return tail->name;

  grent = getgrgid (gid);
  tail = (struct userid *) xmalloc (sizeof (struct userid));
  tail->id.g = gid;
  tail->name = (grent ? xstrdup (grent->gr_name) : NULL);

  /* Add to the head of the list, so most recently used is first.  */
  tail->next = group_alist;
  group_alist = tail;
  return tail->name;
}

#ifdef NOT_USED

/* Translate GROUP to a GID, with cache.
   Return NULL if there is no such group.
   (We also cache which group names have no group entry,
   so we don't keep looking them up.)  */

gid_t *
getgidbyname (group)
     const char *group;
{
  register struct userid *tail;
  struct group *grent;

  for (tail = group_alist; tail; tail = tail->next)
    /* Avoid a function call for the most common case.  */
    if (tail->name && *tail->name == *group && !strcmp (tail->name, group))
      return &tail->id.g;

  for (tail = nogroup_alist; tail; tail = tail->next)
    /* Avoid a function call for the most common case.  */
    if (*tail->name == *group && !strcmp (tail->name, group))
      return 0;

  grent = getgrnam (group);

  tail = (struct userid *) xmalloc (sizeof (struct userid));
  tail->name = xstrdup (group);

  /* Add to the head of the list, so most recently used is first.  */
  if (grent)
    {
      tail->id.g = grent->gr_gid;
      tail->next = group_alist;
      group_alist = tail;
      return &tail->id.g;
    }

  tail->next = nogroup_alist;
  nogroup_alist = tail;
  return 0;
}

#endif /* NOT_USED */
