/* Define an at-style functions like fstatat, unlinkat, fchownat, etc.
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

#define CALL_FUNC(F)				\
  (AT_FUNC_USE_F1_COND				\
    ? AT_FUNC_F1 (F AT_FUNC_POST_FILE_ARGS)	\
    : AT_FUNC_F2 (F AT_FUNC_POST_FILE_ARGS))

/* Call AT_FUNC_F1 or AT_FUNC_F2 (testing AT_FUNC_USE_F1_COND to
   determine which) to operate on FILE, which is in the directory
   open on descriptor FD.  If possible, do it without changing the
   working directory.  Otherwise, resort to using save_cwd/fchdir,
   then AT_FUNC_F?/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.  */
int
AT_FUNC_NAME (int fd, char const *file AT_FUNC_POST_FILE_PARAM_DECLS)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return CALL_FUNC (file);

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = CALL_FUNC (proc_file);
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  if (fchdir (fd) != 0)
    {
      saved_errno = errno;
      free_cwd (&saved_cwd);
      errno = saved_errno;
      return -1;
    }

  err = CALL_FUNC (file);
  saved_errno = (err < 0 ? errno : 0);

  if (restore_cwd (&saved_cwd) != 0)
    openat_restore_fail (errno);

  free_cwd (&saved_cwd);

  if (saved_errno)
    errno = saved_errno;
  return err;
}
#undef CALL_FUNC
