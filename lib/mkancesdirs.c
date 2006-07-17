/* Make a file's ancestor directories.

   Copyright (C) 2006 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "mkancesdirs.h"

#include <errno.h>
#include <sys/stat.h>

#include "dirname.h"
#include "stat-macros.h"

/* Return 0 if FILE is a directory, otherwise -1 (setting errno).  */

static int
test_dir (char const *file)
{
  struct stat st;
  if (stat (file, &st) == 0)
    {
      if (S_ISDIR (st.st_mode))
	return 0;
      errno = ENOTDIR;
    }
  return -1;
}

/* Ensure that the ancestor directories of FILE exist, using an
   algorithm that should work even if two processes execute this
   function in parallel.  Temporarily modify FILE by storing '\0'
   bytes into it, to access the ancestor directories.

   Create any ancestor directories that don't already exist, by
   invoking MAKE_DIR (ANCESTOR, MAKE_DIR_ARG).  This function should
   return zero if successful, -1 (setting errno) otherwise.

   If successful, return 0 with FILE set back to its original value;
   otherwise, return -1 (setting errno), storing a '\0' into *FILE so
   that it names the ancestor directory that had problems.  */

int
mkancesdirs (char *file,
	     int (*make_dir) (char const *, void *),
	     void *make_dir_arg)
{
  /* This algorithm is O(N**2) but in typical practice the fancier
     O(N) algorithms are slower.  */

  /* Address of the previous directory separator that follows an
     ordinary byte in a file name in the left-to-right scan, or NULL
     if no such separator precedes the current location P.  */
  char *sep = NULL;

  char const *prefix_end = file + FILE_SYSTEM_PREFIX_LEN (file);
  char *p;
  char c;

  /* Search backward through FILE using mkdir to create the
     furthest-away ancestor that is needed.  This loop isn't needed
     for correctness, but typically ancestors already exist so this
     loop speeds things up a bit.

     This loop runs a bit faster if errno initially contains an error
     number corresponding to a failed access to FILE.  However, things
     work correctly regardless of errno's initial value.  */

  for (p = last_component (file); prefix_end < p; p--)
    if (ISSLASH (*p) && ! ISSLASH (p[-1]))
      {
	*p = '\0';

	if (errno == ENOENT && make_dir (file, make_dir_arg) == 0)
	  {
	    *p = '/';
	    break;
	  }

	if (errno != ENOENT)
	  {
	    if (test_dir (file) == 0)
	      {
		*p = '/';
		break;
	      }
	    if (errno != ENOENT)
	      return -1;
	  }

	*p = '/';
      }

  /* Scan forward through FILE, creating directories along the way.
     Try mkdir before stat, so that the procedure works even when two
     or more processes are executing it in parallel.  */

  while ((c = *p++))
    if (ISSLASH (*p))
      {
	if (! ISSLASH (c))
	  sep = p;
      }
    else if (ISSLASH (c) && *p && sep)
      {
	*sep = '\0';
	if (make_dir (file, make_dir_arg) != 0 && test_dir (file) != 0)
	  return -1;
	*sep = '/';
      }


  return 0;
}
