/* mkdir-p.c -- Ensure that a directory and its parents exist.

   Copyright (C) 1990, 1997, 1998, 1999, 2000, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

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

/* Written by Paul Eggert, David MacKenzie, and Jim Meyering.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "mkdir-p.h"

#include <errno.h>
#include <sys/stat.h>

#include "gettext.h"
#define _(msgid) gettext (msgid)

#include "dirchownmod.c"
#include "error.h"
#include "quote.h"
#include "mkancesdirs.h"
#include "stat-macros.h"

/* Ensure that the directory DIR exists.

   If MAKE_ANCESTOR is not null, create any ancestor directories that
   don't already exist, by invoking MAKE_ANCESTOR (ANCESTOR, OPTIONS).
   This function should return zero if successful, -1 (setting errno)
   otherwise.  In this case, DIR may be modified by storing '\0' bytes
   into it, to access the ancestor directories, and this modification
   is retained on return if the ancestor directories could not be
   created.

   Create DIR as a new directory with using mkdir with permissions
   MODE.  It is also OK if MAKE_ANCESTOR_DIR is not null and a
   directory DIR already exists.

   Call ANNOUNCE (DIR, OPTIONS) just after successfully making DIR,
   even if some of the following actions fail.

   Set DIR's owner to OWNER and group to GROUP, but leave the owner
   alone if OWNER is (uid_t) -1, and similarly for GROUP.

   Set DIR's mode bits to MODE, except preserve any of the bits that
   correspond to zero bits in MODE_BITS.  In other words, MODE_BITS is
   a mask that specifies which of DIR's mode bits should be set or
   cleared.  MODE should be a subset of MODE_BITS, which in turn
   should be a subset of CHMOD_MODE_BITS.  Changing the mode in this
   way is necessary if DIR already existed or if MODE and MODE_BITS
   specify non-permissions bits like S_ISUID.

   However, if PRESERVE_EXISTING is true and DIR already exists,
   do not attempt to set DIR's ownership and file mode bits.

   This implementation assumes the current umask is zero.

   Return true if DIR exists as a directory with the proper ownership
   and file mode bits when done.  Report a diagnostic and return false
   on failure, storing '\0' into *DIR if an ancestor directory had
   problems.  */

bool
make_dir_parents (char *dir,
		  int (*make_ancestor) (char const *, void *),
		  void *options,
		  mode_t mode,
		  void (*announce) (char const *, void *),
		  mode_t mode_bits,
		  uid_t owner,
		  gid_t group,
		  bool preserve_existing)
{
  bool made_dir = (mkdir (dir, mode) == 0);

  if (!made_dir && make_ancestor && errno == ENOENT)
    {
      if (mkancesdirs (dir, make_ancestor, options) == 0)
	made_dir = (mkdir (dir, mode) == 0);
      else
	{
	  /* mkancestdirs updated DIR for a better-looking
	     diagnostic, so don't try to stat DIR below.  */
	  make_ancestor = NULL;
	}
    }

  if (made_dir)
    {
      announce (dir, options);
      preserve_existing =
	(owner == (uid_t) -1 && group == (gid_t) -1
	 && ! ((mode_bits & (S_ISUID | S_ISGID)) | (mode & S_ISVTX)));
    }
  else
    {
      int mkdir_errno = errno;
      struct stat st;
      if (! (make_ancestor && mkdir_errno != ENOENT
	     && stat (dir, &st) == 0 && S_ISDIR (st.st_mode)))
	{
	  error (0, mkdir_errno, _("cannot create directory %s"), quote (dir));
	  return false;
	}
    }

  if (! preserve_existing
      && (dirchownmod (dir, (made_dir ? mode : (mode_t) -1),
		       owner, group, mode, mode_bits)
	  != 0))
    {
      error (0, errno,
	     _(owner == (uid_t) -1 && group == (gid_t) -1
	       ? "cannot change permissions of %s"
	       : "cannot change owner and permissions of %s"),
	     quote (dir));
      return false;
    }

  return true;
}
