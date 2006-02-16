/* provide a replacement openat function
   Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "openat.h"

#include <stdarg.h>
#include <stddef.h>
#include <errno.h>

#include "dirname.h" /* solely for definition of IS_ABSOLUTE_FILE_NAME */
#include "fcntl--.h"
#include "lstat.h"
#include "openat-priv.h"
#include "save-cwd.h"
#include "unistd--.h"

/* Replacement for Solaris' openat function.
   <http://www.google.com/search?q=openat+site:docs.sun.com>
   First, try to simulate it via open ("/proc/self/fd/FD/FILE").
   Failing that, simulate it by doing save_cwd/fchdir/open/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, upon failure, set errno and return -1, as openat does.
   Upon successful completion, return a file descriptor.  */
int
openat (int fd, char const *file, int flags, ...)
{
  mode_t mode = 0;

  if (flags & O_CREAT)
    {
      va_list arg;
      va_start (arg, flags);

      /* If mode_t is narrower than int, use the promoted type (int),
         not mode_t.  Use sizeof to guess whether mode_t is nerrower;
         we don't know of any practical counterexamples.  */
      if (sizeof (mode_t) < sizeof (int))
	mode = va_arg (arg, int);
      else
	mode = va_arg (arg, mode_t);

      va_end (arg);
    }

  return openat_permissive (fd, file, flags, mode, NULL);
}

/* Like openat (FD, FILE, FLAGS, MODE), but if CWD_ERRNO is
   nonnull, set *CWD_ERRNO to an errno value if unable to save
   or restore the initial working directory.  This is needed only
   the first time remove.c's remove_dir opens a command-line
   directory argument.

   If a previous attempt to restore the current working directory
   failed, then we must not even try to access a `.'-relative name.
   It is the caller's responsibility not to call this function
   in that case.  */

int
openat_permissive (int fd, char const *file, int flags, mode_t mode,
		   int *cwd_errno)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;
  bool save_ok;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return open (file, flags, mode);

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = open (proc_file, flags, mode);
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  save_ok = (save_cwd (&saved_cwd) == 0);
  if (! save_ok)
    {
      if (! cwd_errno)
	openat_save_fail (errno);
      *cwd_errno = errno;
    }

  err = fchdir (fd);
  saved_errno = errno;

  if (! err)
    {
      err = open (file, flags, mode);
      saved_errno = errno;
      if (save_ok && restore_cwd (&saved_cwd) != 0)
	{
	  if (! cwd_errno)
	    openat_restore_fail (errno);
	  *cwd_errno = errno;
	}
    }

  free_cwd (&saved_cwd);
  errno = saved_errno;
  return err;
}

/* Return true if our openat implementation must resort to
   using save_cwd and restore_cwd.  */
bool
openat_needs_fchdir (void)
{
  int fd2;
  int fd = open ("/", O_RDONLY);
  char *proc_file;

  if (fd < 0)
    return true;
  BUILD_PROC_NAME (proc_file, fd, ".");
  fd2 = open (proc_file, O_RDONLY);
  close (fd);
  if (0 <= fd2)
    close (fd2);

  return fd2 < 0;
}

#if !HAVE_FDOPENDIR

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=fdopendir+site:docs.sun.com>
   First, try to simulate it via opendir ("/proc/self/fd/FD").  Failing
   that, simulate it by doing save_cwd/fchdir/opendir(".")/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' fdopendir.

   W A R N I N G:
   Unlike the other fd-related functions here, this one
   effectively consumes its FD parameter.  The caller should not
   close or otherwise manipulate FD if this function returns successfully.  */
DIR *
fdopendir (int fd)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  DIR *dir;

  char *proc_file;
  BUILD_PROC_NAME (proc_file, fd, ".");
  dir = opendir (proc_file);
  saved_errno = errno;

  /* If the syscall fails with an expected errno value, resort to
     save_cwd/restore_cwd.  */
  if (! dir && EXPECTED_ERRNO (saved_errno))
    {
      if (save_cwd (&saved_cwd) != 0)
	openat_save_fail (errno);

      if (fchdir (fd) != 0)
	{
	  dir = NULL;
	  saved_errno = errno;
	}
      else
	{
	  dir = opendir (".");
	  saved_errno = errno;

	  if (restore_cwd (&saved_cwd) != 0)
	    openat_restore_fail (errno);
	}

      free_cwd (&saved_cwd);
    }

  if (dir)
    close (fd);
  errno = saved_errno;
  return dir;
}

#endif

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=fstatat+site:docs.sun.com>
   First, try to simulate it via l?stat ("/proc/self/fd/FD/FILE").
   Failing that, simulate it via save_cwd/fchdir/(stat|lstat)/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' fstatat.  */
int
fstatat (int fd, char const *file, struct stat *st, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return (flag == AT_SYMLINK_NOFOLLOW
	    ? lstat (file, st)
	    : stat (file, st));

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = (flag == AT_SYMLINK_NOFOLLOW
	   ? lstat (proc_file, st)
	   : stat (proc_file, st));
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  err = fchdir (fd);
  saved_errno = errno;

  if (! err)
    {
      err = (flag == AT_SYMLINK_NOFOLLOW
	     ? lstat (file, st)
	     : stat (file, st));
      saved_errno = errno;

      if (restore_cwd (&saved_cwd) != 0)
	openat_restore_fail (errno);
    }

  free_cwd (&saved_cwd);
  errno = saved_errno;
  return err;
}

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=unlinkat+site:docs.sun.com>
   First, try to simulate it via (unlink|rmdir) ("/proc/self/fd/FD/FILE").
   Failing that, simulate it via save_cwd/fchdir/(unlink|rmdir)/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely,
   and usually indicative of a problem that deserves close attention),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' unlinkat.  */
int
unlinkat (int fd, char const *file, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return (flag == AT_REMOVEDIR ? rmdir (file) : unlink (file));

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = (flag == AT_REMOVEDIR ? rmdir (proc_file) : unlink (proc_file));
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  err = fchdir (fd);
  saved_errno = errno;

  if (! err)
    {
      err = (flag == AT_REMOVEDIR ? rmdir (file) : unlink (file));
      saved_errno = errno;

      if (restore_cwd (&saved_cwd) != 0)
	openat_restore_fail (errno);
    }

  free_cwd (&saved_cwd);
  errno = saved_errno;
  return err;
}

/* Replacement for Solaris' function by the same name.
   Invoke chown or lchown on file, FILE, using OWNER and GROUP, in the
   directory open on descriptor FD.  If FLAG is AT_SYMLINK_NOFOLLOW, then
   use lchown, otherwise, use chown.  If possible, do it without changing
   the working directory.  Otherwise, resort to using save_cwd/fchdir,
   then mkdir/restore_cwd.  If either the save_cwd or the restore_cwd
   fails, then give a diagnostic and exit nonzero.  */
int
fchownat (int fd, char const *file, uid_t owner, gid_t group, int flag)
{
  struct saved_cwd saved_cwd;
  int saved_errno;
  int err;

  if (fd == AT_FDCWD || IS_ABSOLUTE_FILE_NAME (file))
    return (flag == AT_SYMLINK_NOFOLLOW
	    ? lchown (file, owner, group)
	    : chown (file, owner, group));

  {
    char *proc_file;
    BUILD_PROC_NAME (proc_file, fd, file);
    err = (flag == AT_SYMLINK_NOFOLLOW
	   ? lchown (proc_file, owner, group)
	   : chown (proc_file, owner, group));
    /* If the syscall succeeds, or if it fails with an unexpected
       errno value, then return right away.  Otherwise, fall through
       and resort to using save_cwd/restore_cwd.  */
    if (0 <= err || ! EXPECTED_ERRNO (errno))
      return err;
  }

  if (save_cwd (&saved_cwd) != 0)
    openat_save_fail (errno);

  err = fchdir (fd);
  saved_errno = errno;

  if (! err)
    {
      err = (flag == AT_SYMLINK_NOFOLLOW
	     ? lchown (file, owner, group)
	     : chown (file, owner, group));
      saved_errno = errno;

      if (restore_cwd (&saved_cwd) != 0)
	openat_restore_fail (errno);
    }

  free_cwd (&saved_cwd);
  errno = saved_errno;
  return err;
}
