/* argmatch.c -- find a match for a string in an array
   Copyright (C) 1990, 1998 Free Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@ai.mit.edu>
   Modified by Akim Demaille <demaille@inf.enst.fr> */

#include "argmatch.h"

#include <stdio.h>
#ifdef STDC_HEADERS
# include <string.h>
#endif

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(Category, Locale) /* empty */
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define bindtextdomain(Domain, Directory) /* empty */
# define textdomain(Domain) /* empty */
# define _(Text) Text
#endif

#ifndef EXIT_BADARG
# define EXIT_BADARG 1
#endif

#include "quotearg.h"

/* When reporting a failing argument, make sure to show invisible
   characters hidden using the quoting style
   ARGMATCH_QUOTING_STYLE. literal_quoting_style is not good.*/

#ifndef ARGMATCH_QUOTING_STYLE
# define ARGMATCH_QUOTING_STYLE c_quoting_style
#endif

#if !HAVE_STRNCASECMP
# include <ctype.h>
/* Compare no more than N characters of S1 and S2,
   ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less
   than, equal to or greater than S2.  */
int
strncasecmp (const char *s1, const char *s2, size_t n)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2 || n == 0)
    return 0;

  do
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
      if (c1 == '\0' || c1 != c2)
        return c1 - c2;
    } while (--n > 0);

  return c1 - c2;
}
#endif

extern char *program_name;

/* If ARG is an unambiguous match for an element of the
   null-terminated array ARGLIST, return the index in ARGLIST
   of the matched element, else -1 if it does not match any element
   or -2 if it is ambiguous (is a prefix of more than one element).
   If SENSITIVE, comparison is case sensitive.

   If VALLIST is none null, use it to resolve ambiguities limited to
   synonyms, i.e., for
     "yes", "yop" -> 0
     "no", "nope" -> 1
   "y" is a valid argument, for `0', and "n" for `1'.  */

static int
__argmatch_internal (const char *arg, const char *const *arglist,
		     const char *vallist, size_t valsize, 
		     int sensitive)
{
  int i;			/* Temporary index in ARGLIST.  */
  size_t arglen;		/* Length of ARG.  */
  int matchind = -1;		/* Index of first nonexact match.  */
  int ambiguous = 0;		/* If nonzero, multiple nonexact match(es).  */

  arglen = strlen (arg);

  /* Test all elements for either exact match or abbreviated matches.  */
  for (i = 0; arglist[i]; i++)
    {
      if (sensitive ? !strncmp (arglist[i], arg, arglen)
	            : !strncasecmp (arglist[i], arg, arglen))
	{
	  if (strlen (arglist[i]) == arglen)
	    /* Exact match found.  */
	    return i;
	  else if (matchind == -1)
	    /* First nonexact match found.  */
	    matchind = i;
	  else
	    /* Second nonexact match found.  */
	    if (vallist == NULL
		|| memcmp (vallist + valsize * matchind, 
			   vallist + valsize * i, valsize))
	      /* There is a real ambiguity, or we could not
                 desambiguise. */
	      ambiguous = 1;
	}
    }
  if (ambiguous)
    return -2;
  else
    return matchind;
}

/* argmatch - case sensitive version */
int
argmatch (const char *arg, const char *const *arglist,
	  const char *vallist, size_t valsize)
{
  return __argmatch_internal (arg, arglist, vallist, valsize, 1);
}

/* argcasematch - case insensitive version */
int
argcasematch (const char *arg, const char *const *arglist,
	      const char *vallist, size_t valsize)
{
  return __argmatch_internal (arg, arglist, vallist, valsize, 0);
}

/* Error reporting for argmatch.
   KIND is a description of the type of entity that was being matched.
   VALUE is the invalid value that was given.
   PROBLEM is the return value from argmatch.  */

void
argmatch_invalid (const char *kind, const char *value, int problem)
{
  enum quoting_style saved_quoting_style;

  /* Make sure to have a good quoting style to report errors.
     literal is insane here. */
  saved_quoting_style = get_quoting_style (NULL);
  set_quoting_style (NULL, ARGMATCH_QUOTING_STYLE);

  /* There is an error */
  fprintf (stderr, "%s: ", program_name);
  if (problem == -1)
    fprintf (stderr,  _("invalid argument %s for `%s'"),
	     quotearg (value), kind);
  else				/* Assume -2.  */
    fprintf (stderr, _("ambiguous argument %s for `%s'"),
	     quotearg (value), kind);
  putc ('\n', stderr);

  set_quoting_style (NULL, saved_quoting_style);
}

/* List the valid arguments for argmatch.
   ARGLIST is the same as in argmatch.
   VALLIST is a pointer to an array of values.
   VALSIZE is the size of the elements of VALLIST */
void
argmatch_valid (const char *const *arglist,
		const char *vallist, size_t valsize)
{
  int i;
  const char * last_val = NULL;

  /* We try to put synonyms on the same line.  The assumption is that
     synonyms follow each other */
  fprintf (stderr, _("Valid arguments are:"));
  for (i = 0 ; arglist[i] ; i++)
    if ((i == 0)
	|| memcmp (last_val, vallist + valsize * i, valsize))
      {
	fprintf (stderr, "\n  - `%s'", arglist[i]);
	last_val = vallist + valsize * i;
      }
    else
      {
	fprintf (stderr, ", `%s'", arglist[i]);
      }
  putc ('\n', stderr);
}

/* Call __argmatch_internal, but handle the error so that it never
   returns.  Errors are reported to the users with a list of valid
   values.

   KIND is a description of the type of entity that was being matched.
   ARG, ARGLIST, and SENSITIVE are the same as in __argmatch_internal
   VALIST, and VALSIZE are the same as in valid_args */
int
__xargmatch_internal (const char *kind, const char *arg,
		      const char *const *arglist, 
		      const char *vallist, size_t valsize,
		      int sensitive)
{
  int i;
  
  i = __argmatch_internal (arg, arglist, vallist, valsize, sensitive);
  if (i >= 0)
    {
      /* Success */
      return i;
    }
  else
    {
      /* Failure */
      argmatch_invalid (kind, arg, i);
      argmatch_valid (arglist, vallist, valsize);
      exit (EXIT_BADARG);
    }
  return -1; 	/* To please some compilers */
}

/* Look for VALUE in VALLIST, an array of objects of size VALSIZE and
   return the first corresponding argument in ARGLIST */
const char *
argmatch_to_argument (char * value,
		      const char *const *arglist,
		      const char *vallist, size_t valsize)
{
  int i;
  
  for (i = 0 ; arglist [i] ; i++)
    if (!memcmp (value, vallist + valsize * i, valsize))
      return arglist [i];
  return NULL;
}

#ifdef TEST
/*
 * Based on "getversion.c" by David MacKenzie <djm@gnu.ai.mit.edu>
 */
char * program_name;
extern const char * getenv ();

/* When to make backup files.  */
enum backup_type
{
  /* Never make backups.  */
  none,

  /* Make simple backups of every file.  */
  simple,

  /* Make numbered backups of files that already have numbered backups,
     and simple backups of the others.  */
  numbered_existing,

  /* Make numbered backups of every file.  */
  numbered
};

/* Two tables describing arguments (keys) and their corresponding
   values */
static const char *const backup_args[] =
{
  "no", "none", "off", 
  "simple", "never", 
  "existing", "nil", 
  "numbered", "t", 
  0
};

static const enum backup_type backup_vals[] =
{
  none, none, none, 
  simple, simple, 
  numbered_existing, numbered_existing, 
  numbered, numbered
};

int
main (int argc, const char *const * argv)
{
  const char * cp;
  enum backup_type backup_type = none;

  program_name = (char *) argv [0];

  if (argc > 2)
    {
      fprintf (stderr, "Usage: %s [VERSION_CONTROL]\n", program_name);
      exit (1);
    }

  if ((cp = getenv ("VERSION_CONTROL")))
    backup_type = XARGCASEMATCH ("$VERSION_CONTROL", cp, 
				 backup_args, backup_vals);

  if (argc == 2)
    backup_type = XARGCASEMATCH (program_name, argv [1], 
				 backup_args, backup_vals);

  printf ("The version control is `%s'\n", 
	  ARGMATCH_TO_ARGUMENT (backup_type, backup_args, backup_vals));

  return 0;
}
#endif
