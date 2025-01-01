#!/bin/sh
# Test for proper exit code of chmod on a processed symlink.

# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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
print_ver_ chmod

mkdir dir || framework_failure_
touch dir/f || framework_failure_
ln -s f dir/l || framework_failure_

# This operation ignores symlinks but should succeed.
chmod u+w -R dir 2> out || fail=1

compare /dev/null out || fail=1

Exit $fail
