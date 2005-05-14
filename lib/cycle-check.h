/* help detect directory cycles efficiently

   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

/* Written by Jim Meyering */

#ifndef CYCLE_CHECK_H
# define CYCLE_CHECK_H 1

# if HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
# include <stdbool.h>
# include "dev-ino.h"

struct cycle_check_state
{
  struct dev_ino dev_ino;
  uintmax_t chdir_counter;
  int magic;
};

void cycle_check_init (struct cycle_check_state *state);
bool cycle_check (struct cycle_check_state *state, struct stat const *sb);

#endif
