#!/bin/sh
# Test test's file related options

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ test

touch file || framework_failure_
chmod 0 file || framework_failure_

cat file && skip_ 'chmod does not limit access'

env test -f file || fail=1
returns_ 1 env test -f fail || fail=1

returns_ 1 env test -r file || fail=1
returns_ 1 env test -x file || fail=1

returns_ 1 env test -d file || fail=1
returns_ 1 env test -s file || fail=1
returns_ 1 env test -S file || fail=1
returns_ 1 env test -c file || fail=1
returns_ 1 env test -b file || fail=1
returns_ 1 env test -p file || fail=1

# Check that 'test' returns true if the file name following "-nt" does not
# resolve.
env test file -nt missing || fail=1
returns_ 1 env test missing -nt file || fail=1

# Check that 'test' returns true if the file name preceding "-ot" does not
# resolve.
env test missing -ot file || fail=1
returns_ 1 env test file -ot missing || fail=1

# Create two files with modification times an hour apart.
t1='2025-10-23 03:00'
t2='2025-10-23 04:00'
touch -m -d "$t1" file1 || framework_failure_
touch -m -d "$t2" file2 || framework_failure_

# Test "-nt" on two existing files.
env test file2 -nt file1 || fail=1
returns_ 1 env test file1 -nt file2 || fail=1

# Test "-ot" on two existing files.
env test file1 -ot file2 || fail=1
returns_ 1 env test file2 -ot file1 || fail=1

# Test "-ef" on files that do not resolve.
returns_ 1 env test missing1 -ef missing2 || fail=1
returns_ 1 env test missing2 -ef missing1 || fail=1
returns_ 1 env test file1 -ef missing1 || fail=1
returns_ 1 env test missing1 -ef file1 || fail=1

# Test "-ef" on normal files.
env test file1 -ef file1 || fail=1
returns_ 1 env test file1 -ef file2 || fail=1

# Test "-ef" on symbolic links.
ln -s file1 symlink1 || framework_failure_
ln -s file2 symlink2 || framework_failure_
env test file1 -ef symlink1 || fail=1
env test symlink1 -ef file1 || fail=1
returns_ 1 env test file1 -ef symlink2 || fail=1
returns_ 1 env test symlink2 -ef file1 || fail=1
returns_ 1 env test symlink1 -ef symlink2 || fail=1
returns_ 1 env test symlink2 -ef symlink1 || fail=1

# Test "-ef" on hard links.
if ln file1 hardlink1 && ln file2 hardlink2; then
  env test file1 -ef hardlink1 || fail=1
  env test hardlink1 -ef file1 || fail=1
  returns_ 1 env test file1 -ef hardlink2 || fail=1
  returns_ 1 env test hardlink2 -ef file1 || fail=1
  returns_ 1 env test hardlink1 -ef hardlink2 || fail=1
  returns_ 1 env test hardlink2 -ef hardlink1 || fail=1
fi

Exit $fail
