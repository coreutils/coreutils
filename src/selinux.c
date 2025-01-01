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

#include <config.h>
#include <selinux/label.h>
#include <selinux/context.h>
#include <sys/types.h>

#include "system.h"
#include "canonicalize.h"
#include "xfts.h"
#include "selinux.h"

#if HAVE_SELINUX_LABEL_H

# if ! HAVE_MODE_TO_SECURITY_CLASS
/*
  This function has been added to libselinux-2.1.12-5, but is here
  for support with older versions of SELinux

  Translates a mode into an Internal SELinux security_class definition.
  Returns 0 on failure, with errno set to EINVAL.
*/
static security_class_t
mode_to_security_class (mode_t m)
{

  if (S_ISREG (m))
    return string_to_security_class ("file");
  if (S_ISDIR (m))
    return string_to_security_class ("dir");
  if (S_ISCHR (m))
    return string_to_security_class ("chr_file");
  if (S_ISBLK (m))
    return string_to_security_class ("blk_file");
  if (S_ISFIFO (m))
    return string_to_security_class ("fifo_file");
  if (S_ISLNK (m))
    return string_to_security_class ("lnk_file");
  if (S_ISSOCK (m))
    return string_to_security_class ("sock_file");

  errno = EINVAL;
  return 0;
}
# endif

/*
  This function takes a PATH and a MODE and then asks SELinux what the label
  of the path object would be if the current process label created it.
  It then returns the label.

  Returns -1 on failure.  errno will be set appropriately.
*/

static int
computecon_raw (char const *path, mode_t mode, char **con)
{
  char *scon_raw = nullptr;
  char *tcon_raw = nullptr;
  security_class_t tclass;
  int rc = -1;

  char *dir = dir_name (path);
  if (!dir)
    goto quit;
  if (getcon_raw (&scon_raw) < 0)
    goto quit;
  if (getfilecon_raw (dir, &tcon_raw) < 0)
    goto quit;
  tclass = mode_to_security_class (mode);
  if (!tclass)
    goto quit;
  rc = security_compute_create_raw (scon_raw, tcon_raw, tclass, con);

 quit:;
  int err = errno;
  free (dir);
  freecon (scon_raw);
  freecon (tcon_raw);
  errno = err;
  return rc;
}

/*
  This function takes a handle, path and mode, it calls computecon_raw to get
  the label of the path object if the current process created it, then it calls
  selabel_lookup to get the default type for the object.  It substitutes the
  default type into label.  It tells the SELinux Kernel to label all new file
  system objects created by the current process with this label.

  Returns -1 on failure.  errno will be set appropriately.
*/
int
defaultcon (struct selabel_handle *selabel_handle,
            char const *path, mode_t mode)
{
  int rc = -1;
  char *scon_raw = nullptr;
  char *tcon_raw = nullptr;
  context_t scontext = 0, tcontext = 0;
  char const *contype;
  char const *constr;
  char *newpath = nullptr;

  if (! IS_ABSOLUTE_FILE_NAME (path))
    {
      /* Generate absolute name as required by subsequent selabel_lookup.  */
      newpath = canonicalize_filename_mode (path, CAN_MISSING);
      if (! newpath)
        goto quit;
      path = newpath;
    }

  if (selabel_lookup_raw (selabel_handle, &scon_raw, path, mode) < 0)
    {
      /* "No such file or directory" is a confusing error,
         when processing files, when in fact it was the
         associated default context that was not found.
         Therefore map the error to something more appropriate
         to the context in which we're using selabel_lookup().  */
      if (errno == ENOENT)
        errno = ENODATA;
      goto quit;
    }
  if (computecon_raw (path, mode, &tcon_raw) < 0)
    goto quit;
  if (!(scontext = context_new (scon_raw)))
    goto quit;
  if (!(tcontext = context_new (tcon_raw)))
    goto quit;

  if (!(contype = context_type_get (scontext)))
    goto quit;
  if (context_type_set (tcontext, contype))
    goto quit;
  if (!(constr = context_str (tcontext)))
    goto quit;

  rc = setfscreatecon_raw (constr);

 quit:;
  int err = errno;
  context_free (scontext);
  context_free (tcontext);
  freecon (scon_raw);
  freecon (tcon_raw);
  free (newpath);
  errno = err;
  return rc;
}

/*
  If SELABEL_HANDLE is null, set PATH's label to the default to the
  local process.  Otherwise use selabel_lookup to determine the
  default label, extract the type field and then modify the file
  system object.  Note only the type field is updated, thus preserving MLS
  levels and user identity etc. of the PATH.

  Returns -1 on failure.  errno will be set appropriately.
*/
static int
restorecon_private (struct selabel_handle *selabel_handle, char const *path)
{
  int rc = -1;
  struct stat sb;
  char *scon_raw = nullptr;
  char *tcon_raw = nullptr;
  context_t scontext = 0, tcontext = 0;
  char const *contype;
  char const *constr;
  int fd;

  if (!selabel_handle)
    {
      if (getfscreatecon_raw (&tcon_raw) < 0)
        return rc;
      if (!tcon_raw)
        {
          errno = ENODATA;
          return rc;
        }
      rc = lsetfilecon_raw (path, tcon_raw);
      int err = errno;
      freecon (tcon_raw);
      errno = err;
      return rc;
    }

  fd = open (path, O_RDONLY | O_NOFOLLOW);
  if (fd == -1 && (errno != ELOOP))
    goto quit;

  if (fd != -1)
    {
      if (fstat (fd, &sb) < 0)
        goto quit;
    }
  else
    {
      if (lstat (path, &sb) < 0)
        goto quit;
    }

  if (selabel_lookup_raw (selabel_handle, &scon_raw, path, sb.st_mode) < 0)
    {
      /* "No such file or directory" is a confusing error,
         when processing files, when in fact it was the
         associated default context that was not found.
         Therefore map the error to something more appropriate
         to the context in which we're using selabel_lookup.  */
      if (errno == ENOENT)
        errno = ENODATA;
      goto quit;
    }
  if (!(scontext = context_new (scon_raw)))
    goto quit;

  if (fd != -1)
    {
      if (fgetfilecon_raw (fd, &tcon_raw) < 0)
        goto quit;
    }
  else
    {
      if (lgetfilecon_raw (path, &tcon_raw) < 0)
        goto quit;
    }

  if (!(tcontext = context_new (tcon_raw)))
    goto quit;

  if (!(contype = context_type_get (scontext)))
    goto quit;
  if (context_type_set (tcontext, contype))
    goto quit;
  if (!(constr = context_str (tcontext)))
    goto quit;

  if (fd != -1)
    rc = fsetfilecon_raw (fd, constr);
  else
    rc = lsetfilecon_raw (path, constr);

 quit:;
  int err = errno;
  if (fd != -1)
    close (fd);
  context_free (scontext);
  context_free (tcontext);
  freecon (scon_raw);
  freecon (tcon_raw);
  errno = err;
  return rc;
}

/*
  This function takes three parameters:

  SELABEL_HANDLE for selabel_lookup, or null to preserve.

  PATH of an existing file system object.

  A RECURSE boolean which if the file system object is a directory, will
  call restorecon_private on every file system object in the directory.

  Return false on failure.  errno will be set appropriately.
*/
bool
restorecon (struct selabel_handle *selabel_handle,
            char const *path, bool recurse)
{
  char *newpath = nullptr;

  if (! IS_ABSOLUTE_FILE_NAME (path))
    {
      /* Generate absolute name as required by subsequent selabel_lookup.
         When RECURSE, this also generates absolute names in the
         fts entries, which may be quicker to process in any case.  */
      newpath = canonicalize_filename_mode (path, CAN_MISSING);
      if (! newpath)
        return false;
      path = newpath;
    }

  if (! recurse)
    {
      bool ok = restorecon_private (selabel_handle, path) != -1;
      int err = errno;
      free (newpath);
      errno = err;
      return ok;
    }

  char const *ftspath[2] = { path, nullptr };
  FTS *fts = xfts_open ((char *const *) ftspath, FTS_PHYSICAL, nullptr);

  int err = 0;
  for (FTSENT *ent; (ent = fts_read (fts)); )
    if (restorecon_private (selabel_handle, fts->fts_path) < 0)
      err = errno;

  if (errno != 0)
    err = errno;

  if (fts_close (fts) != 0)
    err = errno;

  free (newpath);
  return !err;
}
#endif
