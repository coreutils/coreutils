/* Copyright (C) 1991, 1994, 1997, 1998, 2000 Free Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C
   Library.  Bugs can be reported to bug-glibc@prep.ai.mit.edu.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

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

#include <sys/types.h>

/* Include errno.h *after* sys/types.h to work around header problems
   on AIX 3.2.5.  */
#include <errno.h>

/* Don't include stdlib.h because some (e.g., Solaris 2.7) declare putenv
   with a non-const argument.  That would conflict with the declaration of
   rpl_putenv below (due to the #define putenv rpl_putenv from config.h).  */

char *malloc ();
void free ();

#if defined (__GNU_LIBRARY__) || defined (HAVE_STRING_H)
# include <string.h>
#endif
#if defined (__GNU_LIBRARY__) || defined (HAVE_UNISTD_H)
# include <unistd.h>
#endif

#if !defined (__GNU_LIBRARY__) && !defined (HAVE_STRCHR)
# define strchr index
#endif
#if !defined (__GNU_LIBRARY__) && !defined (HAVE_MEMCPY)
# define memcpy(d,s,n) bcopy ((s), (d), (n))
#endif

#if HAVE_GNU_LD
# define environ __environ
#else
extern char **environ;
#endif

#ifndef NULL
# define NULL 0
#endif


/* Put STRING, which is of the form "NAME=VALUE", in the environment.  */
int
rpl_putenv (const char *string)
{
  const char *const name_end = strchr (string, '=');
  register size_t size;
  register char **ep;

  if (name_end == NULL)
    {
      /* Remove the variable from the environment.  */
      size = strlen (string);
      for (ep = environ; *ep != NULL; ++ep)
	if (!strncmp (*ep, string, size) && (*ep)[size] == '=')
	  {
	    while (ep[1] != NULL)
	      {
		ep[0] = ep[1];
		++ep;
	      }
	    *ep = NULL;
	    return 0;
	  }
    }

  size = 0;
  for (ep = environ; *ep != NULL; ++ep)
    if (!strncmp (*ep, string, name_end - string) &&
	(*ep)[name_end - string] == '=')
      break;
    else
      ++size;

  if (*ep == NULL)
    {
      static char **last_environ = NULL;
      char **new_environ = (char **) malloc ((size + 2) * sizeof (char *));
      if (new_environ == NULL)
	return -1;
      (void) memcpy ((void *) new_environ, (void *) environ,
		     size * sizeof (char *));
      new_environ[size] = (char *) string;
      new_environ[size + 1] = NULL;
      if (last_environ != NULL)
	free ((void *) last_environ);
      last_environ = new_environ;
      environ = new_environ;
    }
  else
    *ep = (char *) string;

  return 0;
}
