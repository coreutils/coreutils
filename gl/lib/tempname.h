/* Create a temporary file or directory.

   Copyright (C) 2006 Free Software Foundation, Inc.

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

/* header written by Eric Blake */

/* In gnulib, always prefer large files.  GT_FILE maps to
   __GT_BIGFILE, not __GT_FILE, for a reason.  */
#define GT_FILE		1
#define GT_DIR		2
#define GT_NOCREATE	3

/* Generate a temporary file name based on TMPL.  TMPL must match the
   rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
   does not exist at the time of the call to gen_tempname.  TMPL is
   overwritten with the result.

   KIND may be one of:
   GT_NOCREATE:		simply verify that the name does not exist
			at the time of the call.
   GT_FILE:		create a large file using open(O_CREAT|O_EXCL)
			and return a read-write fd.  The file is mode 0600.
   GT_DIR:		create a directory, which will be mode 0700.

   We use a clever algorithm to get hard-to-predict names. */
extern int gen_tempname (char *tmpl, int kind);
