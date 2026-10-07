#!/bin/sh
# Show that we've eliminated most of ls' failing getxattr syscalls,
# regardless of how many files are in a directory we list.

# Copyright (C) 2012-2026 Free Software Foundation, Inc.

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
uses_strace_

# Create a few files:
seq 20 | xargs touch || framework_failure_

# run the test:
strace -o strace.out \
  -e trace=getxattr,lgetxattr \
  -e fault=getxattr,lgetxattr:error=EOPNOTSUPP \
  ls --color=always -l .>/dev/null
ret=$?

grep getxattr strace.out > getxattr.out || framework_failure_
test -s getxattr.out || skip_ 'getxattr and lgetxattr are not intercepted'

# Ensure that there were no more than 3 *getxattr calls.
test $(wc -l <getxattr.out) -le 3 || fail=1

test "$ret" = 0 || fail=1
Exit $fail
