/* mountlist.h -- declarations for list of mounted filesystems
   Copyright (C) 1991, 1992, 1998, 2000-2002 Free Software Foundation, Inc.

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
# define ME_DUMMY(Fs_name, Fs_type) \
    (!strcmp (Fs_type, "autofs") \
     /* for Irix 6.5 */ \
     || !strcmp (Fs_type, "ignore"))
#endif

#undef STREQ
#define STREQ(a, b) (strcmp ((a), (b)) == 0)

#ifndef ME_REMOTE
/* A file system is `remote' if its Fs_name contains a `:'
   or if (it is of type smbfs and its Fs_name starts with `//').  */
# define ME_REMOTE(Fs_name, Fs_type)	\
    (strchr ((Fs_name), ':') != 0	\
     || ((Fs_name)[0] == '/'		\
	 && (Fs_name)[1] == '/'		\
	 && STREQ (Fs_type, "smbfs")))
#endif
