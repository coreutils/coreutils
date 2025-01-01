#!/bin/sh
# Ensure symlinks are quoted appropriately

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
print_ver_ ls

ln -s 'needs quoting' symlink || framework_failure_

ls -l --quoting-style='shell-escape' symlink >out || fail=1

# Coreutils v8.26 and 8.27 failed to quote the target name
grep "symlink -> 'needs quoting'\$" out >/dev/null ||
  { cat out; fail=1; }

Exit $fail
