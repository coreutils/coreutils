/* Run with a larger (mmap'd) stack.

   Copyright (C) 2003 Free Software Foundation, Inc.

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

#if HAVE_MMAP_STACK
# define RUN_WITH_BIG_STACK_2(F, A, B)					\
    do									\
      {									\
	run_on_mmaped_stack ((void (*) (void)) F, 2, A, B);		\
	error (0, errno, _("warning: unable to use large stack"));	\
	F (A, B);							\
      }									\
    while (0)
#else
# define RUN_WITH_BIG_STACK_2(F, A, B) \
    do { F (A, B); } while (0)
#endif

void run_on_mmaped_stack (void (*func_) (void), size_t argc_, ...);
