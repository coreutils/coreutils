/* unlinkdir.c - determine (and maybe change) whether we can unlink directories

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
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Paul Eggert and Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "unlinkdir.h"

#if HAVE_PRIV_H
# include <priv.h>
#endif
#include <unistd.h>

#if ! UNLINK_CANNOT_UNLINK_DIR

/* Return true if we cannot unlink directories, false if we might be
   able to unlink directories.  If possible, tell the kernel we don't
   want to be able to unlink directories, so that we can return true.  */

bool
cannot_unlink_dir (void)
{
  static bool initialized;
  static bool cannot;

  if (! initialized)
    {
# if defined PRIV_EFFECTIVE && defined PRIV_SYS_LINKDIR
      /* We might be able to unlink directories if we cannot
	 determine our privileges, or if we have the
	 PRIV_SYS_LINKDIR privilege and cannot delete it.  */
      priv_set_t *pset = priv_allocset ();
      if (pset)
	{
	  cannot =
	    (getppriv (PRIV_EFFECTIVE, pset) == 0
	     && (! priv_ismember (pset, PRIV_SYS_LINKDIR)
		 || (priv_delset (pset, PRIV_SYS_LINKDIR) == 0
		     && setppriv (PRIV_SET, PRIV_EFFECTIVE, pset) == 0)));
	  priv_freeset (pset);
	}
# else
      /* In traditional Unix, only root can unlink directories.  */
      cannot = (geteuid () != 0);
# endif
      initialized = true;
    }

  return cannot;
}

#endif
