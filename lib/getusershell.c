/* getusershell.c -- Return names of valid user shells.
   Copyright (C) 1991, 1997, 2000, 2001 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef SHELLS_FILE
# ifndef __DJGPP__
/* File containing a list of nonrestricted shells, one per line. */
#  define SHELLS_FILE "/etc/shells"
# else
/* This is a horrible kludge.  Isn't there a better way?  */
#  define SHELLS_FILE "/dev/env/DJDIR/etc/shells"
# endif
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <ctype.h>
#include "unlocked-io.h"
#include "xalloc.h"

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif

#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))

static int readname ();

#if ! defined ADDITIONAL_DEFAULT_SHELLS && defined __MSDOS__
# define ADDITIONAL_DEFAULT_SHELLS \
  "c:/dos/command.com", "c:/windows/command.com", "c:/command.com",
#else
# define ADDITIONAL_DEFAULT_SHELLS /* empty */
#endif

/* List of shells to use if the shells file is missing. */
static char const* const default_shells[] =
{
  ADDITIONAL_DEFAULT_SHELLS
  "/bin/sh", "/bin/csh", "/usr/bin/sh", "/usr/bin/csh", NULL
};

/* Index of the next shell in `default_shells' to return.
   0 means we are not using `default_shells'. */
static int default_index = 0;

/* Input stream from the shells file. */
static FILE *shellstream = NULL;

/* Line of input from the shells file. */
static char *line = NULL;

/* Number of bytes allocated for `line'. */
static int line_size = 0;

/* Return an entry from the shells file, ignoring comment lines.
   If the file doesn't exist, use the list in DEFAULT_SHELLS (above).
   In any case, the returned string is in memory allocated through malloc.
   Return NULL if there are no more entries.  */

char *
getusershell ()
{
  if (default_index > 0)
    {
      if (default_shells[default_index])
	/* Not at the end of the list yet.  */
	return xstrdup (default_shells[default_index++]);
      return NULL;
    }

  if (shellstream == NULL)
    {
      shellstream = fopen (SHELLS_FILE, "r");
      if (shellstream == NULL)
	{
	  /* No shells file.  Use the default list.  */
	  default_index = 1;
	  return xstrdup (default_shells[0]);
	}
    }

  while (readname (&line, &line_size, shellstream))
    {
      if (*line != '#')
	return line;
    }
  return NULL;			/* End of file. */
}

/* Rewind the shells file. */

void
setusershell ()
{
  default_index = 0;
  if (shellstream)
    rewind (shellstream);
}

/* Close the shells file. */

void
endusershell ()
{
  if (shellstream)
    {
      fclose (shellstream);
      shellstream = NULL;
    }
}

/* Read a line from STREAM, removing any newline at the end.
   Place the result in *NAME, which is malloc'd
   and/or realloc'd as necessary and can start out NULL,
   and whose size is passed and returned in *SIZE.

   Return the number of characters placed in *NAME
   if some nonempty sequence was found, otherwise 0.  */

static int
readname (name, size, stream)
     char **name;
     int *size;
     FILE *stream;
{
  int c;
  int name_index = 0;

  if (*name == NULL)
    {
      *size = 10;
      *name = (char *) xmalloc (*size);
    }

  /* Skip blank space.  */
  while ((c = getc (stream)) != EOF && ISSPACE (c))
    /* Do nothing. */ ;

  while (c != EOF && !ISSPACE (c))
    {
      (*name)[name_index++] = c;
      while (name_index >= *size)
	{
	  *size *= 2;
	  *name = (char *) xrealloc (*name, *size);
	}
      c = getc (stream);
    }
  (*name)[name_index] = '\0';
  return name_index;
}

#ifdef TEST
main ()
{
  char *s;

  while (s = getusershell ())
    puts (s);
  exit (0);
}
#endif
