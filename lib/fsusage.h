/* fsusage.h -- declarations for filesystem space usage info
   Copyright (C) 1991, 1992, 1997 Free Software Foundation, Inc.

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

/* Space usage statistics for a filesystem.  Blocks are 512-byte. */

#if !defined FSUSAGE_H_
# define FSUSAGE_H_

struct fs_usage
{
  int fsu_blocksize;		/* Size of a block.  */
  uintmax_t fsu_blocks;		/* Total blocks. */
  uintmax_t fsu_bfree;		/* Free blocks available to superuser. */
  uintmax_t fsu_bavail;		/* Free blocks available to non-superuser. */
  int fsu_bavail_top_bit_set;	/* 1 if fsu_bavail represents a value < 0.  */
  uintmax_t fsu_files;		/* Total file nodes. */
  uintmax_t fsu_ffree;		/* Free file nodes. */
};

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

int get_fs_usage PARAMS ((const char *path, const char *disk,
			  struct fs_usage *fsp));

#endif
