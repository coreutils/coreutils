/* extent-scan.c -- core functions for scanning extents
   Copyright (C) 2010-2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Written by Jie Liu (jeff.liu@oracle.com).  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>

#include "system.h"
#include "extent-scan.h"

#ifndef HAVE_FIEMAP
# include "fiemap.h"
#endif

/* Allocate space for struct extent_scan, initialize the entries if
   necessary and return it as the input argument of extent_scan_read().  */
extern void
extent_scan_init (int src_fd, struct extent_scan *scan)
{
  scan->fd = src_fd;
  scan->ei_count = 0;
  scan->scan_start = 0;
  scan->initial_scan_failed = false;
  scan->hit_final_extent = false;
}

#ifdef __linux__
# ifndef FS_IOC_FIEMAP
#  define FS_IOC_FIEMAP _IOWR ('f', 11, struct fiemap)
# endif
/* Call ioctl(2) with FS_IOC_FIEMAP (available in linux 2.6.27) to
   obtain a map of file extents excluding holes.  */
extern bool
extent_scan_read (struct extent_scan *scan)
{
  union { struct fiemap f; char c[4096]; } fiemap_buf;
  struct fiemap *fiemap = &fiemap_buf.f;
  struct fiemap_extent *fm_extents = &fiemap->fm_extents[0];
  enum { count = (sizeof fiemap_buf - sizeof *fiemap) / sizeof *fm_extents };
  verify (count != 0);

  /* This is required at least to initialize fiemap->fm_start,
     but also serves (in mid 2010) to appease valgrind, which
     appears not to know the semantics of the FIEMAP ioctl. */
  memset (&fiemap_buf, 0, sizeof fiemap_buf);

  fiemap->fm_start = scan->scan_start;
  fiemap->fm_flags = FIEMAP_FLAG_SYNC;
  fiemap->fm_extent_count = count;
  fiemap->fm_length = FIEMAP_MAX_OFFSET - scan->scan_start;

  /* Fall back to the standard copy if call ioctl(2) failed for the
     the first time.  */
  if (ioctl (scan->fd, FS_IOC_FIEMAP, fiemap) < 0)
    {
      if (scan->scan_start == 0)
        scan->initial_scan_failed = true;
      return false;
    }

  /* If 0 extents are returned, then more get_extent_table() are not needed.  */
  if (fiemap->fm_mapped_extents == 0)
    {
      scan->hit_final_extent = true;
      return false;
    }

  scan->ei_count = fiemap->fm_mapped_extents;
  scan->ext_info = xnmalloc (scan->ei_count, sizeof (struct extent_info));

  unsigned int i;
  for (i = 0; i < scan->ei_count; i++)
    {
      assert (fm_extents[i].fe_logical <= OFF_T_MAX);

      scan->ext_info[i].ext_logical = fm_extents[i].fe_logical;
      scan->ext_info[i].ext_length = fm_extents[i].fe_length;
      scan->ext_info[i].ext_flags = fm_extents[i].fe_flags;
    }

  i--;
  if (scan->ext_info[i].ext_flags & FIEMAP_EXTENT_LAST)
    {
      scan->hit_final_extent = true;
      return true;
    }

  scan->scan_start = fm_extents[i].fe_logical + fm_extents[i].fe_length;

  return true;
}
#else
extern bool
extent_scan_read (struct extent_scan *scan ATTRIBUTE_UNUSED)
{
  errno = ENOTSUP;
  return false;
}
#endif
