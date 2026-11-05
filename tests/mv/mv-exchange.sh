#!/bin/sh
# Test mv --exchange.

# Copyright 2024-2026 Free Software Foundation, Inc.

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
print_ver_ mv
getlimits_


# Test exchanging files.
touch a || framework_failure_
mkdir b || framework_failure_
if ! mv -T --exchange a b 2>exchange_err; then
  grep "$ENOTSUP" exchange_err || { cat exchange_err; fail=1; }
else
  test -d a || fail=1
  test -f b || fail=1
fi

# Test wrong number of arguments.
touch c || framework_failure_
returns_ 1 mv --exchange a 2>/dev/null || fail=1
returns_ 1 mv --exchange a b c 2>/dev/null || fail=1

# Both files must exist.  Check the behavior with a missing destination.
echo "mv: cannot exchange 'b' and 'd': $ENOENT" >exp-err || framework_failure_
returns_ 1 mv --exchange b d >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1

# Check the behavior with a missing source.
echo "mv: cannot stat 'd': $ENOENT" >exp-err || framework_failure_
returns_ 1 mv --exchange d b >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1

# Test the behavior with a missing destination in a target directory.
mkdir d || framework_failure_
echo "mv: cannot exchange 'b' and 'd/b': $ENOENT" >exp-err || framework_failure_
for opt in '' --target-directory=; do
  returns_ 1 mv --exchange b ${opt}d >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare exp-err err || fail=1
done

Exit $fail
