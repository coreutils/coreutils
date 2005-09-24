/* Report a save- or restore-cwd failure in our openat replacement and then exit.

   Copyright (C) 2005 Free Software Foundation, Inc.

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

#include <stdlib.h>

#include "error.h"
#include "exitfail.h"

#include "gettext.h"
#define _(msgid) gettext (msgid)
#define N_(msgid) msgid

void
openat_save_fail (int errno)
{
  error (exit_failure, errno,
	 _("unable to record current working directory"));

  /* The `noreturn' attribute cannot be applied to error, since it returns
     when its first argument is 0.  To help compilers understand that this
     function does not return, call abort.  Also, the abort is a
     safety feature if exit_failure is 0 (which shouldn't happen).  */
  abort ();
}

void
openat_restore_fail (int errno)
{
  error (exit_failure, errno,
	 _("failed to return to initial working directory"));

  /* As above.  */
  abort ();
}
