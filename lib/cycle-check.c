/* help detect directory cycles efficiently
   Copyright 2003 Free Software Foundation, Inc.

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
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Jim Meyering */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
typedef enum {false = 0, true = 1} bool;
#endif

#include "cycle-check.h"
#include "xalloc.h"

#define SAME_INODE(Stat_buf_1, Stat_buf_2) \
  ((Stat_buf_1).st_ino == (Stat_buf_2).st_ino \
   && (Stat_buf_1).st_dev == (Stat_buf_2).st_dev)

#define CC_MAGIC 9827862

static inline bool
is_power_of_two (unsigned int i)
{
  return (i & (i - 1)) == 0;
}

void
cycle_check_init (struct cycle_check_state *state)
{
  state->chdir_counter = 0;
  state->magic = CC_MAGIC;
}

/* In traversing a directory hierarchy, call this function once for each
   descending chdir call, with SB corresponding to the chdir operand.
   If SB corresponds to a directory that has already been seen,
   return true to indicate that there is a directory cycle.
   Note that this is done `lazily', which means that some of
   the directories in the cycle may be processed twice before
   the cycle is detected.  */

bool
cycle_check (struct cycle_check_state *state, struct stat const *sb)
{
  assert (state->magic == CC_MAGIC);

  /* If the current directory ever happens to be the same
     as the one we last recorded for the cycle detection,
     then it's obviously part of a cycle.  */
  if (state->chdir_counter && SAME_INODE (*sb, state->dev_ino))
    return true;

  /* If the number of `descending' chdir calls is a power of two,
     record the dev/ino of the current directory.  */
  if (is_power_of_two (++(state->chdir_counter)))
    {
      state->dev_ino.st_dev = sb->st_dev;
      state->dev_ino.st_ino = sb->st_ino;
    }

  return false;
}
