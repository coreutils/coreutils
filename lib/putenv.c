/* Copyright (C) 1991 Free Software Foundation, Inc.

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
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <ansidecl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef	HAVE_GNU_LD
#define	__environ	environ
#endif

/* Put STRING, which is of the form "NAME=VALUE", in the environment.  */
int
DEFUN(putenv, (string), CONST char *string)
{
  CONST char *CONST name_end = strchr(string, '=');
  register size_t size;
  register char **ep;

  if (name_end == NULL)
    {
      /* Remove the variable from the environment.  */
      size = strlen(string);
      for (ep = __environ; *ep != NULL; ++ep)
	if (!strncmp(*ep, string, size) && (*ep)[size] == '=')
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
  for (ep = __environ; *ep != NULL; ++ep)
    if (!strncmp(*ep, string, name_end - string) &&
	(*ep)[name_end - string] == '=')
      break;
    else
      ++size;

  if (*ep == NULL)
    {
      static char **last_environ = NULL;
      char **new_environ = (char **) malloc((size + 2) * sizeof(char *));
      if (new_environ == NULL)
	return -1;
      (void) memcpy((PTR) new_environ, (PTR) __environ, size * sizeof(char *));
      new_environ[size] = (char *) string;
      new_environ[size + 1] = NULL;
      if (last_environ != NULL)
	free((PTR) last_environ);
      last_environ = new_environ;
      __environ = new_environ;
    }
  else
    *ep = (char *) string;

  return 0;
}
