/* chown-core.h -- types and prototypes shared by chown and chgrp.
   Copyright (C) 2000 Free Software Foundation.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef CHOWN_CORE_H
# define CHOWN_CORE_H

enum Change_status
{
  CH_NOT_APPLIED,
  CH_SUCCEEDED,
  CH_FAILED,
  CH_NO_CHANGE_REQUESTED
};

enum Verbosity
{
  /* Print a message for each file that is processed.  */
  V_high,

  /* Print a message for each file whose attributes we change.  */
  V_changes_only,

  /* Do not be verbose.  This is the default. */
  V_off
};

struct Chown_option
{
  /* Level of verbosity.  */
  enum Verbosity verbosity;

  /* If nonzero, change the ownership of directories recursively. */
  int recurse;

  /* If nonzero, and the systems has support for it, change the ownership
     of symbolic links rather than any files they point to.  */
  int change_symlinks;

  /* If nonzero, force silence (no error messages). */
  int force_silent;

  /* The name of the user to which ownership of the files is being given. */
  char *user_name;

  /* The name of the group to which ownership of the files is being given. */
  char *group_name;
};

void
chopt_init (struct Chown_option *chopt);

int
change_file_owner PARAMS ((int cmdline_arg, const char *file,
			   uid_t user, gid_t group,
			   uid_t old_user, gid_t old_group,
			   struct Chown_option const *chopt));

#endif /* CHOWN_CORE_H */
