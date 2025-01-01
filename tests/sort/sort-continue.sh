#!/bin/sh
# Tests for file descriptor exhaustion.

# Copyright (C) 2009-2025 Free Software Foundation, Inc.

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
print_ver_ sort

# This script uses 'ulimit -n 7' to limit 'sort' to at most 7 open files:
# stdin, stdout, stderr, two input and one output files when merging,
# and an extra.  The extra is for old-fashioned platforms like Solaris 10
# where opening a temp file also requires opening /dev/urandom to
# calculate the temp file's name.

# Skip the test when running under valgrind.
( ulimit -n 7; sort 3<&- 4<&- 5<&- 6<&- < /dev/null ) \
  || skip_ 'fd-limited sort failed; are you running under valgrind?'

for i in $(seq 31); do
  echo $i | tee -a in > __test.$i || framework_failure_
done

# glob before ulimit to avoid issues on bash 3.2 on OS X 10.6.8 at least
test_files=$(echo __test.*)

(
 ulimit -n 7
 sort -n -m $test_files 3<&- 4<&- 5<&- 6<&- < /dev/null > out
) &&
compare in out ||
  { fail=1; echo 'file descriptor exhaustion not handled' 1>&2; }

echo 32 | tee -a in > in1
(
 ulimit -n 7
 sort -n -m $test_files - 3<&- 4<&- 5<&- 6<&- < in1 > out
) &&
compare in out || { fail=1; echo 'stdin not handled properly' 1>&2; }

Exit $fail
