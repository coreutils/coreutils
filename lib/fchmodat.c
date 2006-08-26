/* Change the protections of file relative to an open directory.
   Copyright (C) 2006 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

#include "openat.h"
#include "dirname.h" /* solely for definition of IS_ABSOLUTE_FILE_NAME */
#include "save-cwd.h"
#include "openat-priv.h"

#ifndef HAVE_LCHMOD
/* Use a different name, to avoid conflicting with any
   system-supplied declaration.  */
# undef lchmod
# define lchmod lchmod_rpl
static int lchmod (char const *f, mode_t m) { errno = ENOSYS; return -1; }
#endif

/* Solaris 10 has no function like this.
   Invoke chmod or lchmod on file, FILE, using mode MODE, in the directory
   open on descriptor FD.  If possible, do it without changing the
   working directory.  Otherwise, resort to using save_cwd/fchdir,
   then mkdir/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.
   Note that an attempt to use a FLAG value of AT_SYMLINK_NOFOLLOW
   on a system without lchmod support causes this function to fail.  */

#define AT_FUNC_NAME fchmodat
#define AT_FUNC_F1 lchmod
#define AT_FUNC_F2 chmod
#define AT_FUNC_USE_F1_COND flag == AT_SYMLINK_NOFOLLOW
#define AT_FUNC_POST_FILE_PARAM_DECLS , mode_t mode, int flag
#define AT_FUNC_POST_FILE_ARGS        , mode
#include "at-func.c"
