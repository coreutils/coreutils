#!/bin/sh
# test the behavior of 'tee --append'

# Copyright (C) 2025 Free Software Foundation, Inc.

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
print_ver_ tee

# Test the short and long option.
for opt in -a --append; do
  echo 'line 1' > inp || framework_failure_
  cat inp > exp || framework_failure_
  # POSIX says: "Processing of at least 13 file operands shall be supported."
  files=$(seq 13)
  # Setup and verify the files.
  tee $files <inp >out || fail=1
  for f in out $files; do
    compare exp $f || fail=1
  done
  echo 'line 2' > inp || framework_failure_
  cat inp >> exp || framework_failure_
  # Only one -a or --append option is needed, but check that 'tee' behaves the
  # same with duplicate options.
  opts=$(for f in $files; do printf -- ' %s %s' $opt $f; done)
  tee $opts < inp > out || framework_failure_
  # Standard output should only have the line we appended.
  compare inp out || fail=1
  # The files should have line 1 and line 2.
  for f in $files; do
    compare exp $f || fail=1
  done
done

Exit $fail
