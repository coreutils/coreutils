/* makepath.c -- Ensure that a directory path exists.

   Copyright (C) 1994, 1995, 1996, 1997, 2000, 2003, 2004 Free
   Software Foundation, Inc.

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

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and Jim Meyering.  */

#include <stdbool.h>
#include <sys/types.h>

bool make_path (const char *argpath,
		mode_t mode,
		mode_t parent_mode,
		uid_t owner,
		gid_t group,
		bool preserve_existing,
		const char *verbose_fmt_string);

bool make_dir (const char *dir,
	       const char *dirpath,
	       mode_t mode,
	       bool *created_dir_p);
