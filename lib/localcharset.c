/* Determine a canonical name for the current locale's character encoding.

   Copyright (C) 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* Written by Bruno Haible <haible@clisp.cons.org>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_LANGINFO_CODESET
# include <langinfo.h>
#else
# if HAVE_SETLOCALE
#  include <locale.h>
# endif
#endif

char *xmalloc ();
char *xrealloc ();

/* Pointer to the contents of the charset.alias file, if it has already been
   read, else NULL.  Its format is:
   ALIAS_1 '\0' CANONICAL_1 '\0' ... ALIAS_n '\0' CANONICAL_n '\0' '\0'  */
static char * volatile charset_aliases;

/* Return a pointer to the contents of the charset.alias file.  */
static const char *
get_charset_aliases ()
{
  char *cp;

  cp = charset_aliases;
  if (cp == NULL)
    {
      FILE *fp;

      fp = fopen (LIBDIR "/" "charset.alias", "r");
      if (fp == NULL)
	/* File not found, treat it as empty.  */
	cp = "";
      else
	{
	  /* Parse the file's contents.  */
	  int c;
	  char buf1[50+1];
	  char buf2[50+1];
	  char *res_ptr = NULL;
	  size_t res_size = 0;
	  size_t l1, l2;

	  for (;;)
	    {
	      c = getc (fp);
	      if (c == EOF)
		break;
	      if (c == '\n' || c == ' ' || c == '\t')
		continue;
	      if (c == '#')
		{
		  /* Skip comment, to end of line.  */
		  do
		    c = getc (fp);
		  while (!(c == EOF || c == '\n'));
		  if (c == EOF)
		    break;
		  continue;
		}
	      ungetc (c, fp);
	      if (fscanf(fp, "%50s %50s", buf1, buf2) < 2)
		break;
	      l1 = strlen (buf1);
	      l2 = strlen (buf2);
	      if (res_size == 0)
		{
		  res_size = l1 + 1 + l2 + 1;
		  res_ptr = xmalloc (res_size + 1);
		}
	      else
		{
		  res_size += l1 + 1 + l2 + 1;
		  res_ptr = xrealloc (res_ptr, res_size + 1);
		}
	      strcpy (res_ptr + res_size - (l2 + 1) - (l1 + 1), buf1);
	      strcpy (res_ptr + res_size - (l2 + 1), buf2);
	    }
	  fclose (fp);
	  if (res_size == 0)
	    cp = "";
	  else
	    {
	      *(res_ptr + res_size) = '\0';
	      cp = res_ptr;
	    }
	}

      charset_aliases = cp;
    }

  return cp;
}

/* Determine the current locale's character encoding, and canonicalize it
   into one of the canonical names listed in config.charset.
   The result must not be freed; it is statically allocated.
   If the canonical name cannot be determined, the result is a non-canonical
   name or NULL.  */

#ifdef STATIC
STATIC
#endif
const char *
locale_charset ()
{
  const char *codeset;
  const char *aliases;

#if HAVE_LANGINFO_CODESET

  /* Most systems support nl_langinfo (CODESET) nowadays.  */
  codeset = nl_langinfo (CODESET);

#else

  /* On old systems which lack it, use setlocale and getenv.  */
  const char *locale = NULL;

# if HAVE_SETLOCALE
  locale = setlocale (LC_CTYPE, NULL);
# endif
  if (locale == NULL)
    {
      locale = getenv ("LC_ALL");
      if (locale == NULL)
	{
	  locale = getenv ("LC_CTYPE");
	  if (locale == NULL)
	    locale = getenv ("LANG");
	}
    }

  /* On some old systems, one used to set locale = "iso8859_1". On others,
     you set it to "language_COUNTRY.charset". In any case, we resolve it
     through the charset.alias file.  */
  codeset = locale;

#endif

  if (codeset != NULL)
    {
      /* Resolve alias. */
      for (aliases = get_charset_aliases ();
	   *aliases != '\0';
	   aliases += strlen (aliases) + 1, aliases += strlen (aliases) + 1)
	if (!strcmp (codeset, aliases))
	  {
	    codeset = aliases + strlen (aliases) + 1;
	    break;
	  }
    }

  return codeset;
}
