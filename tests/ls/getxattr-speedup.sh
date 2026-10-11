#!/bin/sh
# Show that we've eliminated most of ls' failing xattr syscalls,
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

trace_ls_xattrs() {
  strace -o strace.out \
   -e trace=listxattr,llistxattr,getxattr,lgetxattr \
   -e fault=listxattr,llistxattr,getxattr,lgetxattr:error=EOPNOTSUPP \
   ls --color=always -l . >/dev/null
}

# Get a base count
# Note there could be no getxattr calls if built without SELinux
# but there should always be listxattr calls.
trace_ls_xattrs; ret=$?
grep xattr strace.out > xattr.out &&
test -s xattr.out || skip_ 'no xattr calls were intercepted'
test "$ret" = 0 || fail=1
base_count=$(wc -l <xattr.out)

# This should always be the case,
# but let's not enforce to reduce coupling
#test $base_count -le 4 || fail=1

# Create a few files:
seq 20 | xargs touch || framework_failure_

# Ensure the count is independent of the number of files
trace_ls_xattrs || fail=1
grep xattr strace.out > xattr.out || fail=1
new_count=$(wc -l <xattr.out)
test $base_count = $new_count || fail=1

Exit $fail
