/* mountlist.h -- declarations for list of mounted filesystems
   Copyright (C) 1991, 1992, 1998 Free Software Foundation, Inc.

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

/* A mount table entry. */
struct mount_entry
{
  char *me_devname;		/* Device node pathname, including "/dev/". */
  char *me_mountdir;		/* Mount point directory pathname. */
  char *me_type;		/* "nfs", "4.2", etc. */
  dev_t me_dev;			/* Device number of me_mountdir. */
  unsigned int me_dummy : 1;	/* Nonzero for dummy filesystems. */
  unsigned int me_remote : 1;	/* Nonzero for remote fileystems. */
  struct mount_entry *me_next;
};

#ifndef PARAMS
# if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

struct mount_entry *read_filesystem_list PARAMS ((int need_fs_type));

#ifndef ME_DUMMY
# define ME_DUMMY(fs_name, fs_type) \
    (!strcmp (fs_type, "auto") || !strcmp (fs_type, "ignore"))
#endif

#ifndef ME_REMOTE
# define ME_REMOTE(fs_name, fs_type) (strchr (fs_name, ':') != 0)
#endif
