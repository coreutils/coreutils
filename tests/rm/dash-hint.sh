#!/bin/sh
# Test that 'rm -foo' gives a hint to the user in some situations.

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
print_ver_ rm

# Check that the hint is not shown when there isn't file named "-foo".
returns_ 1 rm -foo > out 2> err || fail=1
compare /dev/null out || fail=1
grep -F 'to remove the file' err && fail=1

# Check that the hint is shown when there is file named "-foo".
echo a > -foo || framework_failure_
cat <<\EOF > exp || framework_failure_
Try 'rm ./-foo' to remove the file '-foo'.
Try 'rm --help' for more information.
EOF
returns_ 1 rm -foo > out 2> errt || fail=1
compare /dev/null out || fail=1
# Ignore any invalid option error messages from getopt.
tail -n 2 errt > err || framework_failure_
cmp exp err || fail=1

nl='
'
# Check that the output is shell escaped so it can be copy pasted.
echo a > "-foo${nl}bar" || framework_failure_
cat <<\EOF > exp || framework_failure_
Try 'rm ./'-foo'$'\n''bar'' to remove the file '-foo'$'\n''bar'.
Try 'rm --help' for more information.
EOF
returns_ 1 rm "-foo${nl}bar" > out 2> errt || fail=1
compare /dev/null out || fail=1
# Ignore any invalid option error messages from getopt.
tail -n 2 errt > err || framework_failure_
cmp exp err || fail=1

Exit $fail
