/* GNU's read utmp module.
   Copyright (C) 92, 93, 94, 95, 96, 1997 Free Software Foundation, Inc.

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

/* Written by jla; revised by djm */

#include <config.h>

#include <sys/stat.h>
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
#else
# include <strings.h>
#endif /* STDC_HEADERS || HAVE_STRING_H */

#include "readutmp.h"
#include "error.h"

char *xmalloc ();

STRUCT_UTMP *utmp_contents = 0;

/* Copy UT->ut_name into storage obtained from malloc.  Then remove any
   trailing spaces from the copy, NUL terminate it, and return the copy.  */

char *
extract_trimmed_name (ut)
  const STRUCT_UTMP *ut;
{
  char *p, *trimmed_name;

  trimmed_name = xmalloc (sizeof (ut->ut_name) + 1);
  strncpy (trimmed_name, ut->ut_name, sizeof (ut->ut_name));
  /* Append a trailing space character.  Some systems pad names shorter than
     the maximum with spaces, others pad with NULs.  Remove any spaces.  */
  trimmed_name[sizeof (ut->ut_name)] = ' ';
  p = strchr (trimmed_name, ' ');
  if (p != NULL)
    *p = '\0';
  return trimmed_name;
}

/* Read the utmp file FILENAME into UTMP_CONTENTS and return the
   number of entries it contains. */

int
read_utmp (filename)
  const char *filename;
{
  FILE *utmp;
  struct stat file_stats;
  size_t n_read;
  size_t size;

  if (utmp_contents)
    {
      free (utmp_contents);
      utmp_contents = 0;
    }

  utmp = fopen (filename, "r");
  if (utmp == NULL)
    error (1, errno, "%s", filename);

  fstat (fileno (utmp), &file_stats);
  size = file_stats.st_size;
  if (size > 0)
    utmp_contents = (STRUCT_UTMP *) xmalloc (size);
  else
    {
      fclose (utmp);
      return 0;
    }

  /* Use < instead of != in case the utmp just grew.  */
  n_read = fread (utmp_contents, 1, size, utmp);
  if (ferror (utmp) || fclose (utmp) == EOF
      || n_read < size)
    error (1, errno, "%s", filename);

  return size / sizeof (STRUCT_UTMP);
}
