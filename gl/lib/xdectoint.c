/* Convert decimal strings with bounds checking and exit on error.

   Copyright (C) 2014-2025 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include "xdectoint.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#include <error.h>
#include <quote.h>
#include <xstrtol.h>

/* Parse numeric string N_STR of base BASE, and return the value.
   The value is between MIN and MAX.
   Strings can have multiplicative SUFFIXES if specified.
   On a parse error or out-of-range number, diagnose with N_STR and ERR, and
   exit with status ERR_EXIT if nonzero, EXIT_FAILURE otherwise.
   However, if FLAGS & XTOINT_MIN_QUIET, do not diagnose or exit
   for too-low numbers; return MIN and set errno instead.
   Similarly for XTOINT_MAX_QUIET and too-high numbers and MAX.
   The errno value and diagnostic for out-of-range values depend on
   whether FLAGS & XTOINT_MIN_RANGE and FLAGS & XTOINT_MAX_RANGE are set.  */

__xdectoint_t
__xnumtoint (char const *n_str, int base, __xdectoint_t min, __xdectoint_t max,
             char const *suffixes, char const *err, int err_exit,
             int flags)
{
  __xdectoint_t tnum, r;
  strtol_error s_err = __xstrtol (n_str, nullptr, base, &tnum, suffixes);

  /* Errno value to report if there is an overflow.  */
  int overflow_errno;

  if (s_err != LONGINT_INVALID)
    {
      if (tnum < min)
        {
          r = min;
          overflow_errno = flags & XTOINT_MIN_RANGE ? ERANGE : EOVERFLOW;
          if (s_err == LONGINT_OK)
            s_err = LONGINT_OVERFLOW;
        }
      else if (max < tnum)
        {
          r = max;
          overflow_errno = flags & XTOINT_MAX_RANGE ? ERANGE : EOVERFLOW;
          if (s_err == LONGINT_OK)
            s_err = LONGINT_OVERFLOW;
        }
      else
        {
          r = tnum;
          overflow_errno = EOVERFLOW;
        }
    }

  int e = s_err == LONGINT_OVERFLOW ? overflow_errno : 0;

  if (! (s_err == LONGINT_OK
         || (s_err == LONGINT_OVERFLOW
             && flags & (tnum < 0 ? XTOINT_MIN_QUIET : XTOINT_MAX_QUIET))))
    error (err_exit ? err_exit : EXIT_FAILURE, e, "%s: %s", err, quote (n_str));

  errno = e;
  return r;
}

/* Parse decimal string N_STR, and return the value.
   Exit on parse error or if MIN or MAX are exceeded.
   Strings can have multiplicative SUFFIXES if specified.
   ERR is printed along with N_STR on error.  */

__xdectoint_t
__xdectoint (char const *n_str, __xdectoint_t min, __xdectoint_t max,
             char const *suffixes, char const *err, int err_exit)
{
  return __xnumtoint (n_str, 10, min, max, suffixes, err, err_exit, 0);
}
