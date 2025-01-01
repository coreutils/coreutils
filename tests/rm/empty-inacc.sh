#!/bin/sh
# Ensure that rm -rf removes an empty-and-inaccessible directory.

# Copyright (C) 2006-2025 Free Software Foundation, Inc.

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
print_ver_ rm
skip_if_root_

mkdir -m0 inacc || framework_failure_

# Also exercise the different code path that's taken for a directory
# that is empty (hence removable) and unreadable.
mkdir -m a-r -p a/unreadable || framework_failure_

# This would fail for e.g., coreutils-5.93.
rm -rf inacc || fail=1
test -d inacc && fail=1

# This would fail for e.g., coreutils-5.97.
rm -rf a || fail=1
test -d a && fail=1

# Ensure that using rm -d removes an unreadable empty directory.
mkdir -m a-r unreadable2 || framework_failure_
mkdir -m0 inacc2 || framework_failure_

# These would fail for coreutils-9.1 and prior.
rm -d unreadable2 < /dev/null || fail=1
test -d unreadable2 && fail=1
rm -d inacc2 < /dev/null || fail=1
test -d inacc2 && fail=1

# Test the interactive code paths that are new with 9.2:
mkdir -m0 inacc3 || framework_failure_

echo n | rm ---presume-input-tty -di inacc3 > out 2>&1 || fail=1
# decline: ensure it was not deleted, and the prompt was as expected.
printf "rm: attempt removal of inaccessible directory 'inacc3'? " > exp
test -d inacc3 || fail=1
compare exp out || fail=1

echo y | rm ---presume-input-tty -di inacc3 > out 2>&1 || fail=1
# accept: ensure it **was** deleted, and the prompt was as expected.
test -d inacc3 && fail=1
compare exp out || fail=1

Exit $fail
