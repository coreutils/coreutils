/* GNU's read utmp module.
   Copyright (C) 1992-1999 Free Software Foundation, Inc.

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

#include <stdio.h>

#include <sys/stat.h>
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
#else
# include <strings.h>
#endif /* STDC_HEADERS || HAVE_STRING_H */

#include "readutmp.h"

char *xmalloc ();

/* Copy UT->ut_name into storage obtained from malloc.  Then remove any
   trailing spaces from the copy, NUL terminate it, and return the copy.  */

char *
extract_trimmed_name (const STRUCT_UTMP *ut)
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

/* FIXME: use `#if HAVE_UTMPNAME' instead...  */
#if 0

int
read_utmp (const char *filename, int *n_entries, STRUCT_UTMP **utmp_buf)
{
  int count_utmp = 0;
  int n_read;
  STRUCT_UTMP *u;
  STRUCT_UTMP *uptr;
  STRUCT_UTMP *utmp_contents;

  if (utmpname (filename))
    {
      return 1;
    }

  /* FIXME: going through the list twice is wasteful. */

  /* count the entries in utmp */
  setutent ();
  while ((u = getutent ()) != NULL)
    ++count_utmp;

  if (count_utmp == 0)
    return 0;

  utmp_contents = (STRUCT_UTMP *) xmalloc (count_utmp * sizeof (STRUCT_UTMP));

  /* read the entries in utmp */

  /* FIXME: can this fail? */
  setutent ();

  n_read = 0;
  uptr = utmp_contents;
  while ((u = getutent ()) != NULL)
    {
      ++n_read;
      if (n_read > count_utmp)
	{
	  STRUCT_UTMP *old_utmp_contents = utmp_contents;
	  ++count_utmp;
	  utmp_contents = (STRUCT_UTMP *) xrealloc (utmp_contents,
						    (count_utmp
						     * sizeof (STRUCT_UTMP)));
	  uptr = utmp_contents + (uptr - old_utmp_contents);
	}
      *uptr = *u;
      ++uptr;
    }

  if (n_read != count_utmp)
    utmp_contents = (STRUCT_UTMP *) xrealloc (utmp_contents,
					      n_read * sizeof (STRUCT_UTMP));

  /* FIXME: can this fail? */
  endutent ();

  *n_entries = n_read;
  *utmp_buf = utmp_contents;

  return 0;
}

#else

int
read_utmp (const char *filename, int *n_entries, STRUCT_UTMP **utmp_buf)
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

#endif /* HAVE_UTMPNAME */
