#!/bin/sh
# Validate timeout parameter combinations

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
print_ver_ timeout env
getlimits_


# internal errors are 125, distinct from execution failure

# invalid timeouts
returns_ 125 timeout invalid sleep 0 || fail=1
returns_ 125 timeout ' -0.1' sleep 0 || fail=1
returns_ 125 timeout ' -1e-10000' sleep 0 || fail=1

# invalid kill delay
returns_ 125 timeout --kill-after=invalid 1 sleep 0 || fail=1

# invalid timeout suffix
returns_ 125 timeout 42D sleep 0 || fail=1

# floating point notation
timeout 10.34 sleep 0 || fail=1

# nanoseconds potentially supported
timeout 9.999999999 sleep 0 || fail=1

# round underflow up to 1 ns
returns_ 124 timeout 1e-10000 sleep 10 || fail=1

# invalid signal spec
returns_ 125 timeout --signal=invalid 1 sleep 0 || fail=1

# invalid command
returns_ 126 env . && { returns_ 126 timeout 10 . || fail=1; }

# no such command
returns_ 127 timeout 10 no_such || fail=1

Exit $fail
