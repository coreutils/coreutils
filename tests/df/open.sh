#!/bin/sh
# Test that 'df' opens directory operands to trigger automounts.

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
print_ver_ df
require_strace_ openat
getlimits_

trace_open ()
{
  strace --quiet=all -o trace -e trace=open,openat "$@" >out 2>err
}

trace_open true || skip_ 'strace does not support the required options'

ln -s /dev/null dev_null &&
touch file || framework_failure_

# Make sure that non-directories don't get opened.
for file in file dev_null; do
  trace_open df $file || fail=1
  compare /dev/null err || fail=1
  returns_ 1 grep '^open.*"'$file'",.* = [0-9]' trace || fail=1
done

# Make sure that directories get opened so that automounts get triggered.
mkdir dir || framework_failure_
trace_open df dir || fail=1
compare /dev/null err || fail=1
grep '^open.*"dir",.* = [0-9]' trace || fail=1

# Make sure the errors given for missing files are what we expect.
cat <<EOF >exp-err || framework_failure_
df: missing: $ENOENT
df: missing/f: $ENOENT
EOF
returns_ 1 df missing missing/f >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp-err err || fail=1

Exit $fail
