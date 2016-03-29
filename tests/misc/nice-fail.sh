#!/bin/sh
# Verify that internal failure in nice gives exact status.

# Copyright (C) 2009-2016 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ nice


# These tests verify exact status of internal failure.
nice -n 1 # missing command
test $? = 125 || fail=1
nice --- # unknown option
test $? = 125 || fail=1
nice -n 1a # invalid adjustment
test $? = 125 || fail=1
nice sh -c 'exit 2' # exit status propagation
test $? = 2 || fail=2
nice . # invalid command
test $? = 126 || fail=1
nice no_such # no such command
test $? = 127 || fail=1

Exit $fail
