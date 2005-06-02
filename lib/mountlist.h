/* mountlist.h -- declarations for list of mounted file systems

   Copyright (C) 1991, 1992, 1998, 2000, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

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

#ifndef MOUNTLIST_H_
# define MOUNTLIST_H_

# include <stdbool.h>
# include <sys/types.h>

/* A mount table entry. */
struct mount_entry
{
  char *me_devname;		/* Device node name, including "/dev/". */
  char *me_mountdir;		/* Mount point directory name. */
  char *me_type;		/* "nfs", "4.2", etc. */
  dev_t me_dev;			/* Device number of me_mountdir. */
  unsigned int me_dummy : 1;	/* Nonzero for dummy file systems. */
  unsigned int me_remote : 1;	/* Nonzero for remote fileystems. */
  unsigned int me_type_malloced : 1; /* Nonzero if me_type was malloced. */
  struct mount_entry *me_next;
};

struct mount_entry *read_file_system_list (bool need_fs_type);

#endif
