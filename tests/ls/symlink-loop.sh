#!/bin/sh
# Exercise ls symlink ELOOP handling

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

ln -s loop loop || framework_failure_
cat loop >/dev/null 2>&1 && skip_ 'symlink loops not detected'

# With coreutils <= 9.3 we would dereference symlinks on the command line
# and thus fail to display a symlink that could not be traversed.
ls loop || fail=1
ls -l loop || fail=1
ls -l --color=always loop || fail=1

# Ensure these still fail
returns_ 2 ls -H loop || fail=1
returns_ 2 ls -L loop || fail=1

Exit $fail
