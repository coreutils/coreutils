#!/bin/sh
# Test touch --mode.
# Copyright (C) 2026 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ touch

# Use a restrictive but predictable umask where necessary.
# --mode should specify the mode used when creating a new file.

# Test basic creation with mode 0644.
rm -f file || framework_failure_
touch --mode=0644 file || fail=1
test "$(stat -c %a file)" = 644 || fail=1


# Test creation with executable permissions.
rm -f file || framework_failure_
touch --mode=0755 file || fail=1
test "$(stat -c %a file)" = 755 || fail=1


# Test private file permissions.
rm -f file || framework_failure_
touch --mode=0700 file || fail=1
test "$(stat -c %a file)" = 700 || fail=1


# --mode must not change permissions of an existing file.
rm -f file || framework_failure_
touch --mode=0600 file || fail=1
test "$(stat -c %a file)" = 600 || fail=1

touch --mode=0777 file || fail=1
test "$(stat -c %a file)" = 600 || fail=1


# Test that umask is honored when creating files.
rm -f file || framework_failure_
(
  umask 022
  touch --mode=0777 file
) || fail=1

test "$(stat -c %a file)" = 755 || fail=1


# With umask 000, the requested permissions should be preserved.
rm -f file || framework_failure_
(
  umask 000
  touch --mode=0777 file
) || fail=1

test "$(stat -c %a file)" = 777 || fail=1


# Invalid octal modes must be rejected.
for mode in 888 hello -755 10000; do
  rm -f file || framework_failure_

  touch --mode="$mode" file 2>/dev/null && fail=1

  # An invalid mode must not create the file.
  test ! -e file || fail=1
done


# Normal touch behavior must remain unchanged.
rm -f normal || framework_failure_
touch normal || fail=1
test -f normal || fail=1


Exit $fail