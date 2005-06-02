/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef CANONICALIZE_H_
# define CANONICALIZE_H_

enum canonicalize_mode_t
  {
    /* All components must exist.  */
    CAN_EXISTING = 0,

    /* All components excluding last one must exist.  */
    CAN_ALL_BUT_LAST = 1,

    /* No requirements on components existence.  */
    CAN_MISSING = 2
  };
typedef enum canonicalize_mode_t canonicalize_mode_t;

char *canonicalize_filename_mode (const char *, canonicalize_mode_t);

# if !HAVE_CANONICALIZE_FILE_NAME
char *canonicalize_file_name (const char *);
# endif

#endif /* !CANONICALIZE_H_ */
