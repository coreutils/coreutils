#!/bin/sh
# make sure rmdir's --ignore-fail-on-non-empty option works

# Copyright (C) 1999-2025 Free Software Foundation, Inc.

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
print_ver_ rmdir

cwd=$(pwd)
mkdir -p "$cwd/a/b/c" "$cwd/a/x" || framework_failure_

rmdir -p --ignore-fail-on-non-empty "$cwd/a/b/c" || fail=1
# $cwd/a/x should remain
test -d "$cwd/a/x" || fail=1
# $cwd/a/b and $cwd/a/b/c should be gone
test -d "$cwd/a/b" && fail=1
test -d "$cwd/a/b/c" && fail=1

# Ensure that with --ignore-fail-on-non-empty, we still fail, e.g., for EPERM.
# Between 6.11 and 8.31, the following rmdir would mistakenly succeed.
mkdir -p x/y || framework_failure_
chmod a-w x || framework_failure_

if ! uid_is_privileged_; then  # root does not get EPERM.
  returns_ 1 rmdir --ignore-fail-on-non-empty x/y || fail=1
fi

test -d x/y || fail=1
# Between 6.11 and 8.31, the following rmdir would mistakenly fail,
# and also give a nondescript error
touch x/y/z || framework_failure_
rmdir --ignore-fail-on-non-empty x/y || fail=1
test -d x/y || fail=1

if ! uid_is_privileged_; then  # root does not get EPERM.
  # assume empty dir if unreadable entries (so failure to remove diagnosed)
  rm x/y/z || framework_failure_
  chmod a-r x/y || framework_failure_
  returns_ 1 rmdir --ignore-fail-on-non-empty x/y || fail=1
  test -d x/y || fail=1
fi

Exit $fail
