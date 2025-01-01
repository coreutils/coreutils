#!/bin/sh
# Ensure groups processes all arguments before exiting.
# With coreutils-2.27 and prior, it would exit upon first failure.

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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
print_ver_ groups

returns_ 1 groups :1 :2 :3 2> err || fail=1
test $(wc -l < err) = 3 || fail=1

Exit $fail
