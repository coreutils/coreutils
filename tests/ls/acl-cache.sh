#!/bin/sh
# A failed security-context lookup must not disable ACL detection.
# This failed from v9.6 to v9.12 inclusive.

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
print_ver_ ls
require_strace_ getxattr,listxattr
require_setfacl_

grep '^#define USE_ACL 1' $CONFIG_HEADER > /dev/null ||
  skip_ 'insufficient ACL support'

touch acl plain || framework_failure_
uid=$(id -u) || framework_failure_
setfacl -m "u:$uid:rw" acl && setfacl -b plain || framework_failure_

ls -l acl > exp || framework_failure_
grep '^..........+' exp > /dev/null || skip_ 'ls does not report ACLs'

# Require injection to affect only the selected path.
strace --quiet=all -o /dev/null -P plain \
  -e fault=listxattr,llistxattr:error=EIO ls -l acl > out 2> err &&
compare exp out || skip_ 'strace does not support path-filtered injection'

# Ensure a failed getxattr doesn't induce failure to lookup ACLs.
# Also check the fallback when listxattr fails with E2BIG.
for list_fault in '' '-e fault=listxattr,llistxattr:error=E2BIG'; do
  strace --quiet=all -o trace -P plain \
    -e trace=listxattr,llistxattr,getxattr,lgetxattr \
    -e fault=getxattr,lgetxattr:error=EOPNOTSUPP \
    $list_fault ls -l acl plain acl > out 2> err || fail=1
  test "$DEBUG" && cat trace
  compare /dev/null err || fail=1
  test "$(grep -c '^..........+' out)" = 2 || fail=1
done

grep ' (INJECTED)' trace > /dev/null ||
  skip_ 'strace did not inject the requested failure'

Exit $fail
