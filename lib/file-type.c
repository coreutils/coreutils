/* return a string describing the type of a file
   Copyright (C) 2002 Free Software Foundation, Inc.

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

/* Extracted from diffutils/src/diff.c.  */

#include "file-type.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(text) gettext (text)
#else
# define _(text) text
#endif

char const *
file_type (struct stat const *st)
{
  /* See POSIX 1003.1-2001 for these formats.

     To keep diagnostics grammatical in English, the returned string
     must start with a consonant.  */

  if (S_ISREG (st->st_mode))
    return st->st_size == 0 ? _("regular empty file") : _("regular file");

  if (S_ISDIR (st->st_mode))
    return _("directory");

  if (S_ISBLK (st->st_mode))
    return _("block special file");

  if (S_ISCHR (st->st_mode))
    return _("character special file");

  if (S_ISFIFO (st->st_mode))
    return _("fifo");

  if (S_ISLNK (st->st_mode))
    return _("symbolic link");

  if (S_ISSOCK (st->st_mode))
    return _("socket");

  if (S_TYPEISMQ (st))
    return _("message queue");

  if (S_TYPEISSEM (st))
    return _("semaphore");

  if (S_TYPEISSHM (st))
    return _("shared memory object");

  return _("weird file");
}
