/* makepath.c -- Ensure that a directory path exists.

   Copyright (C) 1994, 1995, 1996, 1997, 2000, 2003 Free Software
   Foundation, Inc.

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

#include <sys/types.h>

int make_path (const char *_argpath,
	       int _mode,
	       int _parent_mode,
	       uid_t _owner,
	       gid_t _group,
	       int _preserve_existing,
	       const char *_verbose_fmt_string);

int make_dir (const char *dir,
	      const char *dirpath,
	      mode_t mode,
	      int *created_dir_p);
