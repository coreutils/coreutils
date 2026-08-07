/* Load libselinux on demand.
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

#include <config.h>

#include <dlfcn.h>
#include <selinux/context.h>
#include <selinux/label.h>
#include <selinux/selinux.h>

#include "system.h"

#if HAVE_SYSTEMD_SD_DLOPEN_H
# include <systemd/sd-dlopen.h>

SD_ELF_NOTE_DLOPEN ("selinux",
                    "Support for SELinux",
                    SD_ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED,
                    LIBSELINUX_SONAME);
#endif

/* Gnulib redirects these names to wrappers that validate the returned
   contexts.  This file provides the underlying libselinux entry points.  */
#undef getfilecon
#undef getfilecon_raw
#undef lgetfilecon
#undef lgetfilecon_raw
#undef fgetfilecon
#undef fgetfilecon_raw

extern int getfilecon (char const *, char **);
extern int getfilecon_raw (char const *, char **);
extern int lgetfilecon (char const *, char **);
extern int lgetfilecon_raw (char const *, char **);
extern int fgetfilecon (int, char **);
extern int fgetfilecon_raw (int, char **);

/* Pointers to the libselinux functions resolved by load_libselinux.  */
struct selinux_operations
{
  __typeof__ (&is_selinux_enabled) is_selinux_enabled;
  __typeof__ (&getcon) getcon;
  __typeof__ (&getcon_raw) getcon_raw;
  __typeof__ (&freecon) freecon;
  __typeof__ (&getfscreatecon_raw) getfscreatecon_raw;
  __typeof__ (&setfscreatecon) setfscreatecon;
  __typeof__ (&setfscreatecon_raw) setfscreatecon_raw;
  __typeof__ (&getfilecon) getfilecon;
  __typeof__ (&getfilecon_raw) getfilecon_raw;
  __typeof__ (&lgetfilecon) lgetfilecon;
  __typeof__ (&lgetfilecon_raw) lgetfilecon_raw;
  __typeof__ (&fgetfilecon) fgetfilecon;
  __typeof__ (&fgetfilecon_raw) fgetfilecon_raw;
  __typeof__ (&setfilecon) setfilecon;
  __typeof__ (&lsetfilecon) lsetfilecon;
  __typeof__ (&lsetfilecon_raw) lsetfilecon_raw;
  __typeof__ (&fsetfilecon) fsetfilecon;
  __typeof__ (&fsetfilecon_raw) fsetfilecon_raw;
  __typeof__ (&setexeccon) setexeccon;
  __typeof__ (&security_check_context) security_check_context;
  __typeof__ (&security_compute_create) security_compute_create;
  __typeof__ (&security_compute_create_raw) security_compute_create_raw;
  __typeof__ (&string_to_security_class) string_to_security_class;
#if HAVE_MODE_TO_SECURITY_CLASS
  __typeof__ (&mode_to_security_class) mode_to_security_class;
#endif
#if HAVE_SELINUX_LABEL_H
  __typeof__ (&selabel_open) selabel_open;
  __typeof__ (&selabel_lookup) selabel_lookup;
  __typeof__ (&selabel_lookup_raw) selabel_lookup_raw;
  __typeof__ (&selabel_close) selabel_close;
  __typeof__ (&selinux_file_context_cmp) selinux_file_context_cmp;
#endif
#if HAVE_SELINUX_CONTEXT_H
  __typeof__ (&context_new) context_new;
  __typeof__ (&context_str) context_str;
  __typeof__ (&context_free) context_free;
  __typeof__ (&context_type_get) context_type_get;
  __typeof__ (&context_type_set) context_type_set;
  __typeof__ (&context_user_get) context_user_get;
  __typeof__ (&context_user_set) context_user_set;
  __typeof__ (&context_role_get) context_role_get;
  __typeof__ (&context_role_set) context_role_set;
  __typeof__ (&context_range_get) context_range_get;
  __typeof__ (&context_range_set) context_range_set;
#endif
};

static struct selinux_operations selinux_ops;

/* Return a function pointer in HANDLE for SYMBOL.  */
static void *
symbol_address (void *handle, char const *symbol)
{
  void *address = dlsym (handle, symbol);
  if (!address)
    errno = ENOSYS;
  return address;
}

/* Load libselinux and resolve all required symbols.  Cache the result.
  Return 0 on success.  Return -1 with errno set to ENOSYS on failure.  */
static int
load_libselinux (void)
{
  static int status;
  static void *handle;
  int flags = RTLD_LAZY | RTLD_LOCAL;

  if (status)
    {
      if (status < 0)
        errno = ENOSYS;
      return status < 0 ? -1 : 0;
    }

#ifdef RTLD_NODELETE
  flags |= RTLD_NODELETE;
#endif
  handle = dlopen (LIBSELINUX_SONAME, flags);
  if (!handle)
    {
      status = -1;
      errno = ENOSYS;
      return -1;
    }

  selinux_ops.is_selinux_enabled =
    symbol_address (handle, "is_selinux_enabled");
  if (!selinux_ops.is_selinux_enabled)
    goto fail;
  selinux_ops.getcon = symbol_address (handle, "getcon");
  if (!selinux_ops.getcon)
    goto fail;
  selinux_ops.getcon_raw = symbol_address (handle, "getcon_raw");
  if (!selinux_ops.getcon_raw)
    goto fail;
  selinux_ops.freecon = symbol_address (handle, "freecon");
  if (!selinux_ops.freecon)
    goto fail;
  selinux_ops.getfscreatecon_raw =
    symbol_address (handle, "getfscreatecon_raw");
  if (!selinux_ops.getfscreatecon_raw)
    goto fail;
  selinux_ops.setfscreatecon = symbol_address (handle, "setfscreatecon");
  if (!selinux_ops.setfscreatecon)
    goto fail;
  selinux_ops.setfscreatecon_raw =
    symbol_address (handle, "setfscreatecon_raw");
  if (!selinux_ops.setfscreatecon_raw)
    goto fail;
  selinux_ops.getfilecon = symbol_address (handle, "getfilecon");
  if (!selinux_ops.getfilecon)
    goto fail;
  selinux_ops.getfilecon_raw = symbol_address (handle, "getfilecon_raw");
  if (!selinux_ops.getfilecon_raw)
    goto fail;
  selinux_ops.lgetfilecon = symbol_address (handle, "lgetfilecon");
  if (!selinux_ops.lgetfilecon)
    goto fail;
  selinux_ops.lgetfilecon_raw = symbol_address (handle, "lgetfilecon_raw");
  if (!selinux_ops.lgetfilecon_raw)
    goto fail;
  selinux_ops.fgetfilecon = symbol_address (handle, "fgetfilecon");
  if (!selinux_ops.fgetfilecon)
    goto fail;
  selinux_ops.fgetfilecon_raw = symbol_address (handle, "fgetfilecon_raw");
  if (!selinux_ops.fgetfilecon_raw)
    goto fail;
  selinux_ops.setfilecon = symbol_address (handle, "setfilecon");
  if (!selinux_ops.setfilecon)
    goto fail;
  selinux_ops.lsetfilecon = symbol_address (handle, "lsetfilecon");
  if (!selinux_ops.lsetfilecon)
    goto fail;
  selinux_ops.lsetfilecon_raw = symbol_address (handle, "lsetfilecon_raw");
  if (!selinux_ops.lsetfilecon_raw)
    goto fail;
  selinux_ops.fsetfilecon = symbol_address (handle, "fsetfilecon");
  if (!selinux_ops.fsetfilecon)
    goto fail;
  selinux_ops.fsetfilecon_raw = symbol_address (handle, "fsetfilecon_raw");
  if (!selinux_ops.fsetfilecon_raw)
    goto fail;
  selinux_ops.setexeccon = symbol_address (handle, "setexeccon");
  if (!selinux_ops.setexeccon)
    goto fail;
  selinux_ops.security_check_context =
    symbol_address (handle, "security_check_context");
  if (!selinux_ops.security_check_context)
    goto fail;
  selinux_ops.security_compute_create =
    symbol_address (handle, "security_compute_create");
  if (!selinux_ops.security_compute_create)
    goto fail;
  selinux_ops.security_compute_create_raw =
    symbol_address (handle, "security_compute_create_raw");
  if (!selinux_ops.security_compute_create_raw)
    goto fail;
  selinux_ops.string_to_security_class =
    symbol_address (handle, "string_to_security_class");
  if (!selinux_ops.string_to_security_class)
    goto fail;
#if HAVE_MODE_TO_SECURITY_CLASS
  selinux_ops.mode_to_security_class =
    symbol_address (handle, "mode_to_security_class");
  if (!selinux_ops.mode_to_security_class)
    goto fail;
#endif
#if HAVE_SELINUX_LABEL_H
  selinux_ops.selabel_open = symbol_address (handle, "selabel_open");
  if (!selinux_ops.selabel_open)
    goto fail;
  selinux_ops.selabel_lookup = symbol_address (handle, "selabel_lookup");
  if (!selinux_ops.selabel_lookup)
    goto fail;
  selinux_ops.selabel_lookup_raw =
    symbol_address (handle, "selabel_lookup_raw");
  if (!selinux_ops.selabel_lookup_raw)
    goto fail;
  selinux_ops.selabel_close = symbol_address (handle, "selabel_close");
  if (!selinux_ops.selabel_close)
    goto fail;
  selinux_ops.selinux_file_context_cmp =
    symbol_address (handle, "selinux_file_context_cmp");
  if (!selinux_ops.selinux_file_context_cmp)
    goto fail;
#endif
#if HAVE_SELINUX_CONTEXT_H
  selinux_ops.context_new = symbol_address (handle, "context_new");
  if (!selinux_ops.context_new)
    goto fail;
  selinux_ops.context_str = symbol_address (handle, "context_str");
  if (!selinux_ops.context_str)
    goto fail;
  selinux_ops.context_free = symbol_address (handle, "context_free");
  if (!selinux_ops.context_free)
    goto fail;
  selinux_ops.context_type_get = symbol_address (handle, "context_type_get");
  if (!selinux_ops.context_type_get)
    goto fail;
  selinux_ops.context_type_set = symbol_address (handle, "context_type_set");
  if (!selinux_ops.context_type_set)
    goto fail;
  selinux_ops.context_user_get = symbol_address (handle, "context_user_get");
  if (!selinux_ops.context_user_get)
    goto fail;
  selinux_ops.context_user_set = symbol_address (handle, "context_user_set");
  if (!selinux_ops.context_user_set)
    goto fail;
  selinux_ops.context_role_get = symbol_address (handle, "context_role_get");
  if (!selinux_ops.context_role_get)
    goto fail;
  selinux_ops.context_role_set = symbol_address (handle, "context_role_set");
  if (!selinux_ops.context_role_set)
    goto fail;
  selinux_ops.context_range_get =
    symbol_address (handle, "context_range_get");
  if (!selinux_ops.context_range_get)
    goto fail;
  selinux_ops.context_range_set =
    symbol_address (handle, "context_range_set");
  if (!selinux_ops.context_range_set)
    goto fail;
#endif

  status = 1;
  return 0;

fail:
  dlclose (handle);
  handle = NULL;
  status = -1;
  errno = ENOSYS;
  return -1;
}

#define LOAD_OR_RETURN(Failure) \
  do \
    { \
      if (load_libselinux () < 0) \
        return Failure; \
    } \
  while (false)

extern int
is_selinux_enabled (void)
{
  return load_libselinux () < 0 ? 0 : selinux_ops.is_selinux_enabled ();
}

extern int
getcon (char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.getcon (con);
}

extern int
getcon_raw (char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.getcon_raw (con);
}

extern void
freecon (char *con)
{
  if (con && load_libselinux () == 0)
    selinux_ops.freecon (con);
}

extern int
getfscreatecon_raw (char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.getfscreatecon_raw (con);
}

extern int
setfscreatecon (char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.setfscreatecon (con);
}

extern int
setfscreatecon_raw (char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.setfscreatecon_raw (con);
}

extern int
getfilecon (char const *file, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.getfilecon (file, con);
}

extern int
getfilecon_raw (char const *file, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.getfilecon_raw (file, con);
}

extern int
lgetfilecon (char const *file, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.lgetfilecon (file, con);
}

extern int
lgetfilecon_raw (char const *file, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.lgetfilecon_raw (file, con);
}

extern int
fgetfilecon (int fd, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.fgetfilecon (fd, con);
}

extern int
fgetfilecon_raw (int fd, char **con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.fgetfilecon_raw (fd, con);
}

extern int
setfilecon (char const *file, char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.setfilecon (file, con);
}

extern int
lsetfilecon (char const *file, char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.lsetfilecon (file, con);
}

extern int
lsetfilecon_raw (char const *file, char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.lsetfilecon_raw (file, con);
}

extern int
fsetfilecon (int fd, char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.fsetfilecon (fd, con);
}

extern int
fsetfilecon_raw (int fd, char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.fsetfilecon_raw (fd, con);
}

extern int
setexeccon (char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.setexeccon (con);
}

extern int
security_check_context (char const *con)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.security_check_context (con);
}

extern int
security_compute_create (char const *scon, char const *tcon,
                         security_class_t tclass, char **newcon)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.security_compute_create (scon, tcon, tclass, newcon);
}

extern int
security_compute_create_raw (char const *scon, char const *tcon,
                             security_class_t tclass, char **newcon)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.security_compute_create_raw (scon, tcon, tclass, newcon);
}

extern security_class_t
string_to_security_class (char const *name)
{
  LOAD_OR_RETURN (0);
  return selinux_ops.string_to_security_class (name);
}

#if HAVE_MODE_TO_SECURITY_CLASS
extern security_class_t
mode_to_security_class (mode_t mode)
{
  LOAD_OR_RETURN (0);
  return selinux_ops.mode_to_security_class (mode);
}
#endif

#if HAVE_SELINUX_LABEL_H
extern struct selabel_handle *
selabel_open (unsigned int backend, struct selinux_opt const *opts,
              unsigned int nopts)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.selabel_open (backend, opts, nopts);
}

extern int
selabel_lookup (struct selabel_handle *handle, char **con,
                char const *key, int type)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.selabel_lookup (handle, con, key, type);
}

extern int
selabel_lookup_raw (struct selabel_handle *handle, char **con,
                    char const *key, int type)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.selabel_lookup_raw (handle, con, key, type);
}

extern void
selabel_close (struct selabel_handle *handle)
{
  if (handle && load_libselinux () == 0)
    selinux_ops.selabel_close (handle);
}

extern int
selinux_file_context_cmp (char const *a, char const *b)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.selinux_file_context_cmp (a, b);
}
#endif

#if HAVE_SELINUX_CONTEXT_H
extern context_t
context_new (char const *con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_new (con);
}

extern char const *
context_str (context_t con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_str (con);
}

extern void
context_free (context_t con)
{
  if (con && load_libselinux () == 0)
    selinux_ops.context_free (con);
}

extern char const *
context_type_get (context_t con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_type_get (con);
}

extern int
context_type_set (context_t con, char const *value)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.context_type_set (con, value);
}

extern char const *
context_user_get (context_t con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_user_get (con);
}

extern int
context_user_set (context_t con, char const *value)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.context_user_set (con, value);
}

extern char const *
context_role_get (context_t con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_role_get (con);
}

extern int
context_role_set (context_t con, char const *value)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.context_role_set (con, value);
}

extern char const *
context_range_get (context_t con)
{
  LOAD_OR_RETURN (NULL);
  return selinux_ops.context_range_get (con);
}

extern int
context_range_set (context_t con, char const *value)
{
  LOAD_OR_RETURN (-1);
  return selinux_ops.context_range_set (con, value);
}
#endif

#undef LOAD_OR_RETURN
