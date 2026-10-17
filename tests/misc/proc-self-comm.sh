#!/bin/sh
# Test that the program name in /proc/self/comm is properly set.

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
print_ver_ cat

test -f /proc/self/comm || skip_ '/proc/self/comm is required'

# Set up variables to execute 'cat' from its absolute path and,
# if available, through the single binary.
abs_cat=$(cmd_exe cat) || framework_failure_
single_binary=$(cmd_exe coreutils) &&
  single_binary_cat="$single_binary --coreutils-prog=cat"

if test -n "$single_binary_cat" && ! $single_binary_cat /dev/null 2>err; then
  grep "^coreutils: unknown program 'cat'$" err || framework_failure_
  single_binary=
  single_binary_cat=
fi

# If we are not using --enable-single-binary=symlinks, then set
# up a symlink in the test directory to ensure this gets tested.
if test -n "$single_binary" && ! test -L "$abs_cat"; then
  mkdir dir || framework_failure_
  ln -s "$single_binary" dir/cat || framework_failure_
  symlink_cat='env PATH='"$(pwd)/dir:$PATH"' cat'
fi

echo 'cat' >exp || framework_failure_
for prog in cat "$abs_cat" "$single_binary_cat" "$symlink_cat"; do
  test -z "$prog" && continue
  $prog /proc/self/comm >out 2>err || fail=1
  compare exp out || fail=1
  compare /dev/null err || fail=1
done

Exit $fail
