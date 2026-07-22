/* fts-missing.h -- helpers for files missing during an FTS traversal.

   Copyright (C) 2026 Free Software Foundation, Inc.

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

#ifndef FTS_MISSING_H
# define FTS_MISSING_H

# include "fts_.h"

static inline bool
ignore_missing_fts_entry (FTSENT const *ent)
{
  return (FTS_ROOTLEVEL < ent->fts_level
          && ent->fts_info != FTS_SL
          && ent->fts_info != FTS_SLNONE);
}

static inline bool
ignorable_fts_error (FTSENT const *ent)
{
  return (ignorable_traversal_errno (ent->fts_errno)
          && (ent->fts_info == FTS_NS || ent->fts_info == FTS_ERR
              || ent->fts_info == FTS_DNR));
}

#endif /* FTS_MISSING_H */
