/* closeout.c - close standard output
   Copyright (C) 1998 Free Software Foundation, Inc.

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

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include <stdio.h>
#include "closeout.h"
#include "error.h"

/* Close standard output, exiting with status STATUS on failure.  */
void
close_stdout_status (int status)
{
  if (ferror (stdout))
    error (status, 0, _("write error"));
  if (fclose (stdout) != 0)
    error (status, errno, _("write error"));
}

/* Close standard output, exiting with status EXIT_FAILURE on failure.  */
void
close_stdout (void)
{
  close_stdout_status (EXIT_FAILURE);
}
