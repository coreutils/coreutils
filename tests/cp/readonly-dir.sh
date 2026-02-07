#!/bin/sh
# Test cp behavior with readonly directories

# Copyright (C) 2026 Free Software Foundation, Inc.

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
print_ver_ cp
skip_if_setgid_

umask 022

# Test case for readonly directory permission preservation
# This addresses the specific case where 'cp -r' and 'cp -a' should
# preserve readonly directory permissions (555) instead of changing
# them to writable permissions (755).

# Create test directory structure
mkdir -p a/b/c/d || framework_failure_
touch a/b/c/d/bar.txt || framework_failure_
echo "test content" > a/b/c/d/bar.txt || framework_failure_

# Make directories readonly (remove write permissions)
chmod -R -w a || framework_failure_

# Test 1: cp -r should preserve readonly directory permissions
cp -r a b || fail=1

# Check that the root copied directory has readonly permissions
mode_b=$(stat --format=%a b) || fail=1
test "$mode_b" = "555" || fail=1

# Check subdirectories too
mode_bb=$(stat --format=%a b/b) || fail=1
test "$mode_bb" = "555" || fail=1

mode_bbc=$(stat --format=%a b/b/c) || fail=1
test "$mode_bbc" = "555" || fail=1

mode_bbcd=$(stat --format=%a b/b/c/d) || fail=1
test "$mode_bbcd" = "555" || fail=1

# Test 2: cp -a should preserve readonly directory permissions and not fail
cp -a a c || fail=1

# Check that cp -a also preserved readonly permissions
mode_c=$(stat --format=%a c) || fail=1
test "$mode_c" = "555" || fail=1

mode_cb=$(stat --format=%a c/b) || fail=1
test "$mode_cb" = "555" || fail=1

Exit $fail
