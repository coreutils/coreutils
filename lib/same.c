/* Determine whether two file names refer to the same file.
   Copyright (C) 1997-2000 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#ifndef errno
extern int errno;
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include "same.h"
#include "dirname.h"
#include "error.h"
#include "xalloc.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif

#define STREQ(a, b) (strcmp ((a), (b)) == 0)

#ifndef HAVE_DECL_FREE
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_FREE
void free ();
#endif

char *base_name PARAMS ((char const *));

#define SAME_INODE(Stat_buf_1, Stat_buf_2) \
  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

/* Return nonzero if SOURCE and DEST point to the same name in the same
   directory.  */

int
same_name (const char *source, const char *dest)
{
  struct stat source_dir_stats;
  struct stat dest_dir_stats;
  char *source_dirname, *dest_dirname;

  source_dirname = dir_name (source);
  dest_dirname = dir_name (dest);
  if (source_dirname == NULL || dest_dirname == NULL)
    xalloc_die ();

  if (stat (source_dirname, &source_dir_stats))
    {
      /* Shouldn't happen.  */
      error (1, errno, "%s", source_dirname);
    }

  if (stat (dest_dirname, &dest_dir_stats))
    {
      /* Shouldn't happen.  */
      error (1, errno, "%s", dest_dirname);
    }

  free (source_dirname);
  free (dest_dirname);

  return (SAME_INODE (source_dir_stats, dest_dir_stats)
	  && STREQ (base_name (source), base_name (dest)));
}
