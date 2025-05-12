/* selinux - core functions for maintaining SELinux labeling
   Copyright (C) 2012-2025 Free Software Foundation, Inc.

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

/* Written by Daniel Walsh <dwalsh@redhat.com> */

#ifndef COREUTILS_SELINUX_H
# define COREUTILS_SELINUX_H

struct selabel_handle;

/* Return true if ERR corresponds to an unsupported request,
   or if there is no context or it's inaccessible.  */
static inline bool
ignorable_ctx_err (int err)
{
  return err == ENOTSUP || err == ENODATA;
}

# if HAVE_SELINUX_LABEL_H

extern bool
restorecon (struct selabel_handle *selabel_handle,
            char const *path, bool recurse);
extern int
defaultcon (struct selabel_handle *selabel_handle,
            char const *path, mode_t mode);

# else

static inline bool
restorecon (MAYBE_UNUSED struct selabel_handle *selabel_handle,
            MAYBE_UNUSED char const *path, MAYBE_UNUSED bool recurse)
{ errno = ENOTSUP; return false; }

static inline int
defaultcon (MAYBE_UNUSED struct selabel_handle *selabel_handle,
            MAYBE_UNUSED char const *path, MAYBE_UNUSED mode_t mode)
{ errno = ENOTSUP; return -1; }

# endif

#endif
