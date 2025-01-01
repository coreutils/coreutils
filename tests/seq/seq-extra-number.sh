#!/bin/sh
# Test the "print_extra_number" logic seq.c:print_numbers()

# Copyright (C) 2019-2025 Free Software Foundation, Inc.

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
print_ver_ seq

##
## Test 1: the documented reason for the logic
##
cat<<'EOF'>exp1 || framework_failure_
0.000000
0.000001
0.000002
0.000003
EOF

seq 0 0.000001 0.000003 > out1 || fail=1
compare exp1 out1 || fail=1


##
## Test 2: before 8.32, this resulted in TWO lines
## (print_extra_number was erroneously set to true)
## The '=' is there instead of a space to ease visual inspection.
cat<<'EOF'>exp2 || framework_failure_
1e+06=
EOF

seq -f "%g=" 1000000 1000000 > out2 || fail=1
compare exp2 out2 || fail=1

Exit $fail
