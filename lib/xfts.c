/* xfts.c -- a wrapper for fts_open

   Copyright (C) 2003, 2005 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdbool.h>

#include "exit.h"
#include "error.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "quote.h"
#include "xalloc.h"
#include "xfts.h"

/* Fail with a proper diagnostic if fts_open fails.  */

FTS *
xfts_open (char * const *argv, int options,
	   int (*compar) (const FTSENT **, const FTSENT **))
{
  FTS *fts = fts_open (argv, options, compar);
  if (fts == NULL)
    {
      /* This can fail in three ways: out of memory, invalid bit_flags,
	 and one or more of the FILES is an empty string.  We could try
	 to decipher that errno==EINVAL means invalid bit_flags and
	 errno==ENOENT means there's an empty string, but that seems wrong.
	 Ideally, fts_open would return a proper error indicator.  For now,
	 we'll presume that the bit_flags are valid and just check for
	 empty strings.  */
      bool invalid_arg = false;
      for (; *argv; ++argv)
	{
	  if (**argv == '\0')
	    invalid_arg = true;
	}
      if (invalid_arg)
	error (EXIT_FAILURE, 0, _("invalid argument: %s"), quote (""));
      else
	xalloc_die ();
    }

  return fts;
}
