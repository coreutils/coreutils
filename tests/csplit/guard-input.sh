#!/bin/sh
# Ensure csplit doesn't overwrite input with output.

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
print_ver_ csplit

# Larger than csplit's initial read buffer, so that part of the input
# is still unread when an output file is created.
seq 10000 > exp || framework_failure_

# The first output file is created before any input is read:
# csplit truncated the input and failed with "input disappeared".
cp exp xx00 || framework_failure_
returns_ 1 csplit xx00 2 > out 2> err || fail=1
compare exp xx00 || fail=1
compare /dev/null out || fail=1
echo "csplit: 'xx00' would overwrite input; aborting" > experr \
  || framework_failure_
compare experr err || fail=1

# A later output file: csplit lost the unread part of the input and
# still exited successfully.  Now the output files already created
# are removed, but not the input.
rm -f xx?? || framework_failure_
cp exp xx01 || framework_failure_
returns_ 1 csplit xx01 2 > out 2> err || fail=1
compare exp xx01 || fail=1
test -f xx00 && fail=1
echo 2 > expout || framework_failure_
compare expout out || fail=1
echo "csplit: 'xx01' would overwrite input; aborting" > experr \
  || framework_failure_
compare experr err || fail=1

# With -k the output files already created are kept.
rm -f xx?? || framework_failure_
cp exp xx01 || framework_failure_
returns_ 1 csplit -k xx01 2 > /dev/null 2>&1 || fail=1
compare exp xx01 || fail=1
echo 1 > exp00 || framework_failure_
compare exp00 xx00 || fail=1

# The same file under another name, and as standard input.
rm -f xx?? || framework_failure_
cp exp xx01 || framework_failure_
ln -s xx01 in-symlink || framework_failure_
ln xx01 in-hardlink || framework_failure_
returns_ 1 csplit in-symlink 2 > /dev/null 2>&1 || fail=1
returns_ 1 csplit in-hardlink 2 > /dev/null 2>&1 || fail=1
returns_ 1 csplit - 2 < xx01 > /dev/null 2>&1 || fail=1
compare exp xx01 || fail=1

# Output file names from --prefix and --suffix-format.
rm -f xx?? || framework_failure_
cp exp in-01 || framework_failure_
returns_ 1 csplit -f in- in-01 2 > /dev/null 2>&1 || fail=1
compare exp in-01 || fail=1
cp exp xx-1 || framework_failure_
returns_ 1 csplit -b '-%d' xx-1 2 > /dev/null 2>&1 || fail=1
compare exp xx-1 || fail=1

# An existing output file that is not the input is still truncated.
rm -f xx?? || framework_failure_
printf '%1000s\n' x > xx00 || framework_failure_
csplit exp 2 > /dev/null || fail=1
compare exp00 xx00 || fail=1

Exit $fail
