#!/bin/sh
# Ensure ln rejects backup suffixes containing path separators to prevent path traversal
# This test verifies that backup suffixes with '/' are sanitized to prevent writing
# backup files outside the intended directory.

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
print_ver_ ln

touch a b || framework_failure_
mkdir subdir || framework_failure_

# Test 1: Command line suffix with path traversal attempt
# This should NOT create 'c' file due to path traversal in suffix
ln -S '_/../c' -b -s a b || fail=1

# Verify the traversal was blocked - 'c' should not exist
test -f c && fail=1

# Verify backup was created with default suffix instead
test -f b~ || fail=1

rm -f b b~ || framework_failure_

# Test 2: Environment variable suffix with path traversal attempt
# This should also be sanitized
touch b || framework_failure_
SIMPLE_BACKUP_SUFFIX='_/../../malicious' ln -b -s a b || fail=1

# Verify no traversal occurred
test -f malicious && fail=1
test -f ../malicious && fail=1
test -f ../../malicious && fail=1

# Verify backup created with default suffix
test -f b~ || fail=1

rm -f b b~ || framework_failure_

# Test 3: Verify normal suffixes still work
touch b || framework_failure_
ln -S '.backup' -b -s a b || fail=1

# Should create backup with custom suffix
test -f b.backup || fail=1

# Should NOT create backup with default suffix
test -f b~ && fail=1

Exit $fail
