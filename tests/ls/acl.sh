#!/bin/sh
# verify that ls -al with acl displays the "+"

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

require_setfacl_

# Skip this test if ls was built without ACL support:
grep '^#define USE_ACL 1' $CONFIG_HEADER > /dev/null ||
  skip_ "insufficient ACL support"

mkdir k || framework_failure_

setfacl -d -m user::rwx k 2> /dev/null || framework_failure_
getfacl k | grep 'default:' || skip_ 'default ACL not set'

ls_l=$(ls -ld k) || fail=1

case $ls_l in
  d[rwxsStT-]*+\ *) ;;
  *) fail=1; ;;
esac

Exit $fail
