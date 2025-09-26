#!/bin/sh
# Test test's file related options

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ test

touch file || framework_failure_
chmod 0 file || framework_failure_

cat file && skip_ 'chmod does not limit access'

env test -f file || fail=1
returns_ 1 env test -f fail || fail=1

returns_ 1 env test -r file || fail=1
returns_ 1 env test -x file || fail=1

returns_ 1 env test -d file || fail=1
returns_ 1 env test -s file || fail=1
returns_ 1 env test -S file || fail=1
returns_ 1 env test -c file || fail=1
returns_ 1 env test -b file || fail=1
returns_ 1 env test -p file || fail=1

Exit $fail
