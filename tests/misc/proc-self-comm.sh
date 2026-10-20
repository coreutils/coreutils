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
single_binary=$(cmd_exe coreutils)

if test -n "$single_binary" && ! single_binary_build_; then
  warn_ "Warning: ignoring single binary dispatcher: $single_binary"
  single_binary=
fi

mkdir dir || framework_failure_

# If we are not using --enable-single-binary=symlinks, then set
# up a symlink in the test directory to ensure this gets tested.
if test -n "$single_binary" && ! test -L "$abs_cat"; then
  ln -s "$single_binary" dir/cat || framework_failure_
  symlink_cat=dir/cat
  # If we are using --enable-single-binary=shebangs, create a
  # binary to pretend we are using --program-prefix=g.
  if test $(wc -l < "$abs_cat") = 1; then
    ginstall "$abs_cat" dir/gcat || framework_failure_
    gcat=dir/gcat
  fi
fi

echo 'cat' >cat-exp || framework_failure_
echo 'gcat' >gcat-exp || framework_failure_
for prog in cat "$abs_cat" "$single_binary" "$symlink_cat" "$gcat"; do
  test -z "$prog" && continue
  exp=cat-exp
  test "$prog" = "$gcat" && exp=gcat-exp
  single_binary_opt=
  test "$prog" = "$single_binary" && single_binary_opt=--coreutils-prog=cat
  if test "$prog" = "$symlink_cat"; then
    env PATH="$(pwd)/dir:$PATH" cat /proc/self/comm >out 2>err || fail=1
  else
    "$prog" $single_binary_opt /proc/self/comm >out 2>err || fail=1
  fi
  compare $exp out || fail=1
  compare /dev/null err || fail=1
done

Exit $fail
