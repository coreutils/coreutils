/* Check target directories for commands like cp and mv.
   Copyright 2017 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert.  */

#include <config.h>
#include <targetdir.h>

#include <die.h>
#include <dirname.h>
#include <root-uid.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Check whether DIR, which the caller presumably has already verified
   is a directory or a symlink to a directory, is likely to be
   vulnerable as the target directory of a command like 'cp ... DIR'.
   If DIR_LSTAT is nonnull, it is the result of calling lstat on DIR.

   Return TARGETDIR_OK if DIR is OK, which does not necessarily mean
   that DIR is a directory or that it is invulnerable to the attack,
   only that it satisfies the heuristics.  Return TARGETDIR_NOT if DIR
   becomes inaccessible or a non-directory while checking things.
   Return TARGETDIR_VULNERABLE if the heuristics suggest that DIR is a
   likely candidate to be hijacked by a symlink attack.

   This function might temporarily modify the DIR string; it restores
   the string to its original value before returning.  */

enum targetdir
targetdir_operand_type (char *restrict dir,
                        struct stat const *restrict dir_lstat)
{
  char *lc = last_component (dir);
  size_t lclen = strlen (lc);

  /* If DIR ends in / or has a last component of . or .. then it is
     good enough.  */
  if (lclen == 0 || ISSLASH (lc[lclen - 1])
      || strcmp (lc, ".") == 0 || strcmp (lc, "..") == 0)
    return TARGETDIR_OK;

  char lc0 = *lc;
  *lc = '\0';
  struct stat parent_stat;
  bool parent_stat_ok = stat (*dir ? dir : ".", &parent_stat) == 0;
  *lc = lc0;

  /* If DIR's parent cannot be statted, DIR can't be statted either.  */
  if (! parent_stat_ok)
    return TARGETDIR_NOT;

  uid_t euid = geteuid ();
  if (parent_stat.st_uid == ROOT_UID || parent_stat.st_uid == euid)
    {
      /* If no other non-root user can write the parent directory, it
         is safe.  If the parent directory's UID and GID are the same,
         assume the common convention of a single-user group with the
         same ID, to avoid returning TARGETDIR_VULNERABLE when users
         employing this convention have mode-775 directories.  */
      if (! (parent_stat.st_mode
             & (S_IWOTH
                | (parent_stat.st_uid == parent_stat.st_gid ? 0 : S_IWGRP))))
        return TARGETDIR_OK;

      /* If the parent is sticky, and no other non-root user owns
         either the parent or DIR, it should be OK.  Do not follow
         symlinks when checking DIR for this.  */
      if (parent_stat.st_mode & S_ISVTX)
        {
          struct stat st;
          if (!dir_lstat)
            {
              if (lstat (dir, &st) != 0)
                return TARGETDIR_NOT;
              dir_lstat = &st;
            }
          if (dir_lstat->st_uid == ROOT_UID || dir_lstat->st_uid == euid)
            return TARGETDIR_OK;
        }
    }

  return TARGETDIR_VULNERABLE;
}
