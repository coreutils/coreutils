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

/* Read the utmp file FILENAME into *UTMP_BUF, set *N_ENTRIES to the
   number of entries read, and return zero.  If there is any error,
   return non-zero and don't modify the parameters.  */

int
read_utmp (filename, n_entries, utmp_buf)
  const char *filename;
  int *n_entries;
  STRUCT_UTMP **utmp_buf;
{
  FILE *utmp;
  struct stat file_stats;
  size_t n_read;
  size_t size;
  STRUCT_UTMP *buf;

  utmp = fopen (filename, "r");
  if (utmp == NULL)
    return 1;

  fstat (fileno (utmp), &file_stats);
  size = file_stats.st_size;
  if (size > 0)
    buf = (STRUCT_UTMP *) xmalloc (size);
  else
    {
      fclose (utmp);
      return 1;
    }

  /* Use < instead of != in case the utmp just grew.  */
  n_read = fread (buf, 1, size, utmp);
  if (ferror (utmp) || fclose (utmp) == EOF
      || n_read < size)
    return 1;

  *n_entries = size / sizeof (STRUCT_UTMP);
  *utmp_buf = buf;

  return 0;
}
