/* Utility to help print --version output in a consistent format.
   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

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

/* Written by Jim Meyering. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unlocked-io.h"
#include "version-etc.h"
#include "xalloc.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

/* Default copyright goes to the FSF. */

char* version_etc_copyright =
  /* Do *not* mark this string for translation.  */
  "Copyright (C) 2003 Free Software Foundation, Inc.";


/* Like version_etc, below, but with the NULL-terminated author list
   provided via a variable of type va_list.  */
void
version_etc_va (FILE *stream,
		char const *command_name, char const *package,
		char const *version, va_list authors)
{
  unsigned int n_authors;
  va_list saved_authors;

#ifdef __va_copy
  __va_copy (saved_authors, authors);
#else
  saved_authors = authors;
#endif

  for (n_authors = 0; va_arg (authors, char const *); ++n_authors)
    {
      /* empty */
    }
  va_end (authors);

  if (n_authors == 0)
    abort ();

  if (command_name)
    fprintf (stream, "%s (%s) %s\n", command_name, package, version);
  else
    fprintf (stream, "%s %s\n", package, version);

  switch (n_authors)
    {
    case 1:
      vfprintf (stream, _("Written by %s.\n"), saved_authors);
      break;
    case 2:
      vfprintf (stream, _("Written by %s and %s.\n"), saved_authors);
      break;
    case 3:
      vfprintf (stream, _("Written by %s, %s, and %s.\n"), saved_authors);
      break;
    case 4:
      vfprintf (stream, _("Written by %s, %s, %s, and %s.\n"), saved_authors);
      break;
    default:
      {

	/* Note that the following must have one `%s' and one `%%s'. */
#define FMT_TEMPLATE _("Written by %sand %%s.\n")

	/* for the string "%s, %s, ..., %s, "  */
	size_t s_len = (n_authors - 1) * strlen ("%s, ");
	char *s_fmt = xmalloc (s_len + 1);

	/* This could be a few bytes tighter, but don't bother because
	   that'd just make it a little more fragile.  */
	char *full_fmt = xmalloc (strlen (FMT_TEMPLATE) + s_len + 1);

	unsigned int i;
	char *s = s_fmt;
	for (i = 0; i < n_authors - 1; i++)
	  s = stpcpy (s, "%s, ");
	sprintf (full_fmt, FMT_TEMPLATE, s_fmt);
	free (s_fmt);

	vfprintf (stream, full_fmt, saved_authors);
	free (full_fmt);
      }
      break;
    }
  va_end (saved_authors);
  putc ('\n', stream);

  fputs (version_etc_copyright, stream);
  putc ('\n', stream);

  fputs (_("\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"),
	 stream);
}


/* Display the --version information the standard way.

   If COMMAND_NAME is NULL, the PACKAGE is asumed to be the name of
   the program.  The formats are therefore:

   PACKAGE VERSION

   or

   COMMAND_NAME (PACKAGE) VERSION.

   There must be one or more author names (each as a separate string)
   after the VERSION argument, and the final argument must be `NULL'.  */
void
version_etc (FILE *stream,
	     char const *command_name, char const *package,
	     char const *version, ...)
{
  va_list authors;

  va_start (authors, version);
  version_etc_va (stream, command_name, package, version, authors);
}
