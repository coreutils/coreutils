/* fsusage.h -- declarations for filesystem space usage info
   Copyright (C) 1991, 1992 Free Software Foundation, Inc.

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
struct fs_usage
{
  long fsu_blocks;		/* Total blocks. */
  long fsu_bfree;		/* Free blocks available to superuser. */
  long fsu_bavail;		/* Free blocks available to non-superuser. */
  long fsu_files;		/* Total file nodes. */
  long fsu_ffree;		/* Free file nodes. */
};

#ifndef __P
#if defined (__GNUC__) || (defined (__STDC__) && __STDC__)
#define __P(args) args
#else
#define __P(args) ()
#endif  /* GCC.  */
#endif  /* Not __P.  */

int get_fs_usage __P ((const char *path, const char *disk,
		       struct fs_usage *fsp));
