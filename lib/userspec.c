/* userspec.c -- Parse a user and group string.
   Copyright (C) 1989-1992, 1997, 1998, 2000, 2002 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
char *alloca ();
#  endif
# endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "xalloc.h"
#include "xstrtol.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

#ifndef _POSIX_VERSION
struct passwd *getpwnam ();
struct group *getgrnam ();
struct group *getgrgid ();
#endif

#ifndef HAVE_ENDGRENT
# define endgrent() ((void) 0)
#endif

#ifndef HAVE_ENDPWENT
# define endpwent() ((void) 0)
#endif

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

/* The extra casts work around common compiler bugs.  */
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) \
			      ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

#ifndef UID_T_MAX
# define UID_T_MAX TYPE_MAXIMUM (uid_t)
#endif

#ifndef GID_T_MAX
# define GID_T_MAX TYPE_MAXIMUM (gid_t)
#endif

/* MAXUID may come from limits.h or sys/params.h.  */
#ifndef MAXUID
# define MAXUID UID_T_MAX
#endif
#ifndef MAXGID
# define MAXGID GID_T_MAX
#endif

/* Perform the equivalent of the statement `dest = strdup (src);',
   but obtaining storage via alloca instead of from the heap.  */

#define V_STRDUP(dest, src)						\
  do									\
    {									\
      int _len = strlen ((src));					\
      (dest) = (char *) alloca (_len + 1);				\
      strcpy (dest, src);						\
    }									\
  while (0)

/* ISDIGIT differs from isdigit, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   ISDIGIT_LOCALE unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#ifndef strdup
char *strdup ();
#endif

/* Return nonzero if STR represents an unsigned decimal integer,
   otherwise return 0. */

static int
is_number (const char *str)
{
  for (; *str; str++)
    if (!ISDIGIT (*str))
      return 0;
  return 1;
}

/* Extract from NAME, which has the form "[user][:.][group]",
   a USERNAME, UID U, GROUPNAME, and GID G.
   Either user or group, or both, must be present.
   If the group is omitted but the ":" separator is given,
   use the given user's login group.
   If SPEC_ARG contains a `:', then use that as the separator, ignoring
   any `.'s.  If there is no `:', but there is a `.', then first look
   up the entire SPEC_ARG as a login name.  If that look-up fails, then
   try again interpreting the `.'  as a separator.

   USERNAME and GROUPNAME will be in newly malloc'd memory.
   Either one might be NULL instead, indicating that it was not
   given and the corresponding numeric ID was left unchanged.

   Return NULL if successful, a static error message string if not.  */

const char *
parse_user_spec (const char *spec_arg, uid_t *uid, gid_t *gid,
		 char **username_arg, char **groupname_arg)
{
  static const char *E_invalid_user = N_("invalid user");
  static const char *E_invalid_group = N_("invalid group");
  static const char *E_bad_spec =
    N_("cannot get the login group of a numeric UID");
  static const char *E_cannot_omit_both =
    N_("cannot omit both user and group");

  const char *error_msg;
  char *spec;			/* A copy we can write on.  */
  struct passwd *pwd;
  struct group *grp;
  char *g, *u, *separator;
  char *groupname;
  int maybe_retry = 0;
  char *dot = NULL;

  error_msg = NULL;
  *username_arg = *groupname_arg = NULL;
  groupname = NULL;

  V_STRDUP (spec, spec_arg);

  /* Find the POSIX `:' separator if there is one.  */
  separator = strchr (spec, ':');

  /* If there is no colon, then see if there's a `.'.  */
  if (separator == NULL)
    {
      dot = strchr (spec, '.');
      /* If there's no colon but there is a `.', then first look up the
	 whole spec, in case it's an OWNER name that includes a dot.
	 If that fails, then we'll try again, but interpreting the `.'
	 as a separator.  */
      /* FIXME: accepting `.' as the separator is contrary to POSIX.
	 someday we should drop support for this.  */
      if (dot)
	maybe_retry = 1;
    }

 retry:

  /* Replace separator with a NUL.  */
  if (separator != NULL)
    *separator = '\0';

  /* Set U and G to non-zero length strings corresponding to user and
     group specifiers or to NULL.  */
  u = (*spec == '\0' ? NULL : spec);

  g = (separator == NULL || *(separator + 1) == '\0'
       ? NULL
       : separator + 1);

  if (u == NULL && g == NULL)
    return _(E_cannot_omit_both);

#ifdef __DJGPP__
  /* Pretend that we are the user U whose group is G.  This makes
     pwd and grp functions ``know'' about the UID and GID of these.  */
  if (u && !is_number (u))
    setenv ("USER", u, 1);
  if (g && !is_number (g))
    setenv ("GROUP", g, 1);
#endif

  if (u != NULL)
    {
      pwd = getpwnam (u);
      if (pwd == NULL)
	{

	  if (!is_number (u))
	    error_msg = E_invalid_user;
	  else
	    {
	      int use_login_group;
	      use_login_group = (separator != NULL && g == NULL);
	      if (use_login_group)
		error_msg = E_bad_spec;
	      else
		{
		  unsigned long int tmp_long;
		  if (xstrtoul (u, NULL, 0, &tmp_long, NULL) != LONGINT_OK
		      || tmp_long > MAXUID)
		    return _(E_invalid_user);
		  *uid = tmp_long;
		}
	    }
	}
      else
	{
	  *uid = pwd->pw_uid;
	  if (g == NULL && separator != NULL)
	    {
	      /* A separator was given, but a group was not specified,
	         so get the login group.  */
	      *gid = pwd->pw_gid;
	      grp = getgrgid (pwd->pw_gid);
	      if (grp == NULL)
		{
		  /* This is enough room to hold the unsigned decimal
		     representation of any 32-bit quantity and the trailing
		     zero byte.  */
		  char uint_buf[21];
		  sprintf (uint_buf, "%u", (unsigned) (pwd->pw_gid));
		  V_STRDUP (groupname, uint_buf);
		}
	      else
		{
		  V_STRDUP (groupname, grp->gr_name);
		}
	      endgrent ();
	    }
	}
      endpwent ();
    }

  if (g != NULL && error_msg == NULL)
    {
      /* Explicit group.  */
      grp = getgrnam (g);
      if (grp == NULL)
	{
	  if (!is_number (g))
	    error_msg = E_invalid_group;
	  else
	    {
	      unsigned long int tmp_long;
	      if (xstrtoul (g, NULL, 0, &tmp_long, NULL) != LONGINT_OK
		  || tmp_long > MAXGID)
		return _(E_invalid_group);
	      *gid = tmp_long;
	    }
	}
      else
	*gid = grp->gr_gid;
      endgrent ();		/* Save a file descriptor.  */

      if (error_msg == NULL)
	V_STRDUP (groupname, g);
    }

  if (error_msg == NULL)
    {
      if (u != NULL)
	{
	  *username_arg = strdup (u);
	  if (*username_arg == NULL)
	    error_msg = xalloc_msg_memory_exhausted;
	}

      if (groupname != NULL && error_msg == NULL)
	{
	  *groupname_arg = strdup (groupname);
	  if (*groupname_arg == NULL)
	    {
	      if (*username_arg != NULL)
		{
		  free (*username_arg);
		  *username_arg = NULL;
		}
	      error_msg = xalloc_msg_memory_exhausted;
	    }
	}
    }

  if (error_msg && maybe_retry)
    {
      maybe_retry = 0;
      separator = dot;
      error_msg = NULL;
      goto retry;
    }

  return _(error_msg);
}

#ifdef TEST

# define NULL_CHECK(s) ((s) == NULL ? "(null)" : (s))

int
main (int argc, char **argv)
{
  int i;

  for (i = 1; i < argc; i++)
    {
      const char *e;
      char *username, *groupname;
      uid_t uid;
      gid_t gid;
      char *tmp;

      tmp = strdup (argv[i]);
      e = parse_user_spec (tmp, &uid, &gid, &username, &groupname);
      free (tmp);
      printf ("%s: %u %u %s %s %s\n",
	      argv[i],
	      (unsigned int) uid,
	      (unsigned int) gid,
	      NULL_CHECK (username),
	      NULL_CHECK (groupname),
	      NULL_CHECK (e));
    }

  exit (0);
}

#endif
