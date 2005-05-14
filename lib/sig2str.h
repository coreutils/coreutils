/* sig2str.h -- convert between signal names and numbers

   Copyright (C) 2002, 2005 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#include <signal.h>

/* Don't override system declarations of SIG2STR_MAX, sig2str, str2sig.  */
#ifndef SIG2STR_MAX

# include "intprops.h"

/* Size of a buffer needed to hold a signal name like "HUP".  */
# define SIG2STR_MAX (sizeof "SIGRTMAX" + INT_STRLEN_BOUND (int) - 1)

int sig2str (int, char *);
int str2sig (char const *, int *);

#endif

/* An upper bound on signal numbers allowed by the system.  */

#if defined _sys_nsig
# define SIGNUM_BOUND (_sys_nsig - 1)
#elif defined NSIG
# define SIGNUM_BOUND (NSIG - 1)
#else
# define SIGNUM_BOUND 64
#endif
