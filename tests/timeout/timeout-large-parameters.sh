#!/bin/sh
# Validate large timeout parameters
# Separated from standard parameter testing due to kernel overflow bugs.

# Copyright (C) 2008-2025 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ timeout
getlimits_

# It was seen on 32 bit Linux/HPPA and OpenIndiana 11
# that a kernel time_t overflowing cause the timer to fire immediately.
# This is still racy, but unlikely to cause an issue unless
# timeout can't process the timer firing within 3 seconds.
timeout $TIME_T_OFLOW sleep 3
if test $? = 124; then
  skip_ 'timeout $TIME_T_OFLOW ..., timed out immediately!'
fi

# timeout overflow
timeout $UINT_OFLOW sleep 0 || fail=1

# timeout overflow
timeout ${TIME_T_OFLOW}d sleep 0 || fail=1

# floating point notation
timeout 2.34e+5d sleep 0 || fail=1

# floating point overflow
timeout $LDBL_MAX sleep 0 || fail=1
returns_ 125 timeout -- -$LDBL_MAX sleep 0 || fail=1

Exit $fail
