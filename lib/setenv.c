/* Copyright (C) 1992, 1995, 2000 Free Software Foundation, Inc.

NOTE: The canonical source of this file is maintained with the GNU C Library.
Bugs can be reported to bug-glibc@prep.ai.mit.edu.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>

#if _LIBC || HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if _LIBC || HAVE_STRING_H
# include <string.h>
#endif
#if _LIBC || HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef	HAVE_GNU_LD
# define __environ	environ
#endif

int
setenv (name, value, replace)
     const char *name;
     const char *value;
     int replace;
{
  register char **ep;
  register size_t size;
  const size_t namelen = strlen (name);
  const size_t vallen = strlen (value) + 1;

  size = 0;
  for (ep = __environ; *ep != NULL; ++ep)
    if (!strncmp (*ep, name, namelen) && (*ep)[namelen] == '=')
      break;
    else
      ++size;

  if (*ep == NULL)
    {
      static char **last_environ;
      char **new_environ;
      if (__environ == last_environ)
	/* We allocated this space; we can extend it.  */
	new_environ = (char **) realloc (last_environ,
					 (size + 2) * sizeof (char *));
      else
	new_environ = (char **) malloc ((size + 2) * sizeof (char *));

      if (new_environ == NULL)
	return -1;

      new_environ[size] = malloc (namelen + 1 + vallen);
      if (new_environ[size] == NULL)
	{
	  free ((char *) new_environ);
	  errno = ENOMEM;
	  return -1;
	}

      if (__environ != last_environ)
	memcpy ((char *) new_environ, (char *) __environ,
		size * sizeof (char *));

      memcpy (new_environ[size], name, namelen);
      new_environ[size][namelen] = '=';
      memcpy (&new_environ[size][namelen + 1], value, vallen);

      new_environ[size + 1] = NULL;

      last_environ = __environ = new_environ;
    }
  else if (replace)
    {
      size_t len = strlen (*ep);
      if (len + 1 < namelen + 1 + vallen)
	{
	  /* The existing string is too short; malloc a new one.  */
	  char *new = malloc (namelen + 1 + vallen);
	  if (new == NULL)
	    return -1;
	  *ep = new;
	}
      memcpy (*ep, name, namelen);
      (*ep)[namelen] = '=';
      memcpy (&(*ep)[namelen + 1], value, vallen);
    }

  return 0;
}

void
unsetenv (name)
     const char *name;
{
  const size_t len = strlen (name);
  char **ep;

  for (ep = __environ; *ep; ++ep)
    if (!strncmp (*ep, name, len) && (*ep)[len] == '=')
      {
	/* Found it.  Remove this pointer by moving later ones back.  */
	char **dp = ep;
	do
	  dp[0] = dp[1];
	while (*dp++);
	/* Continue the loop in case NAME appears again.  */
      }
}
