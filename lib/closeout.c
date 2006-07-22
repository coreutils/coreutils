/* Close standard output, exiting with a diagnostic on error.

   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2004, 2006 Free
   Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "closeout.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "close-stream.h"
#include "error.h"
#include "exitfail.h"
#include "quotearg.h"

static const char *file_name;

/* Set the file name to be reported in the event an error is detected
   by close_stdout.  */
void
close_stdout_set_file_name (const char *file)
{
  file_name = file;
}

/* Close standard output.  On error, issue a diagnostic and _exit
   with status 'exit_failure'.

   Since close_stdout is commonly registered via 'atexit', POSIX
   and the C standard both say that it should not call 'exit',
   because the behavior is undefined if 'exit' is called more than
   once.  So it calls '_exit' instead of 'exit'.  If close_stdout
   is registered via atexit before other functions are registered,
   the other functions can act before this _exit is invoked.

   Applications that use close_stdout should flush any streams
   other than stdout and stderr before exiting, since the call to
   _exit will bypass other buffer flushing.  Applications should
   be flushing and closing other streams anyway, to check for I/O
   errors.  Also, applications should not use tmpfile, since _exit
   can bypass the removal of these files.

   It's important to detect such failures and exit nonzero because many
   tools (most notably `make' and other build-management systems) depend
   on being able to detect failure in other tools via their exit status.  */

void
close_stdout (void)
{
  if (close_stream (stdout) != 0)
    {
      char const *write_error = _("write error");
      if (file_name)
	error (0, errno, "%s: %s", quotearg_colon (file_name),
	       write_error);
      else
	error (0, errno, "%s", write_error);

      _exit (exit_failure);
    }
}
