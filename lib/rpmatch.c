/* Determine whether string value is affirmation or negative response
   according to current locale's data.
   Copyright (C) 1996, 1998, 2000 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS || _LIBC
# include <stddef.h>
# include <stdlib.h>
#else
# ifndef NULL
#  define NULL 0
# endif
#endif

#if ENABLE_NLS
# include <sys/types.h>
# if HAVE_LIMITS_H
#  include <limits.h>
# endif
# include <regex.h>
# include <libintl.h>
# define _(Text) gettext (Text)

static int
try (const char *response, const char *pattern, const int match,
     const int nomatch, const char **lastp, regex_t *re)
{
  if (pattern != *lastp)
    {
      /* The pattern has changed.  */
      if (*lastp)
	{
	  /* Free the old compiled pattern.  */
	  regfree (re);
	  *lastp = NULL;
	}
      /* Compile the pattern and cache it for future runs.  */
      if (regcomp (re, pattern, REG_EXTENDED) != 0)
	return -1;
      *lastp = pattern;
    }

  /* See if the regular expression matches RESPONSE.  */
  return regexec (re, response, 0, NULL, 0) == 0 ? match : nomatch;
}
#endif


int
rpmatch (const char *response)
{
#if ENABLE_NLS
  /* Match against one of the response patterns, compiling the pattern
     first if necessary.  */

  /* We cache the response patterns and compiled regexps here.  */
  static const char *yesexpr, *noexpr;
  static regex_t yesre, nore;
  int result;

  return ((result = try (response, _("^[yY]"), 1, 0,
			 &yesexpr, &yesre))
	  ? result
	  : try (response, _("^[nN]"), 0, -1, &noexpr, &nore));
#else
  /* Test against "^[yY]" and "^[nN]", hardcoded to avoid requiring regex */
  return (*response == 'y' || *response == 'Y' ? 1
	  : *response == 'n' || *response == 'N' ? 0 : -1);
#endif
}
