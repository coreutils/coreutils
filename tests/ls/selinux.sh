#!/bin/sh
# Test ls SELinux file context output

# Copyright (C) 2024-2025 Free Software Foundation, Inc.

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
require_selinux_

touch file || framework_failure_
ln -s file link || framework_failure_

case $(stat --printf='%C' file) in
  *:*:*:*) ;;
  *) skip_ 'unable to match default security context';;
esac

for f in file link; do

  # ensure that ls -l output includes the "."
  test "$(ls -l $f | cut -c11)" = . || fail=1

  # ensure that ls -lZ output includes context
  ls_output=$(LC_ALL=C ls -lnZ "$f") || fail=1
  set x $ls_output
  case $6 in
    *:*:*:*) ;;
    *) fail=1 ;;
  esac
done

Exit $fail
