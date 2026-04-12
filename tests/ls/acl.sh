#!/bin/sh
# verify that ls -al with acl displays the "+"

# Copyright (C) 2024-2026 Free Software Foundation, Inc.

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

# Verify that the presence of ACLs doesn't break the display
mkdir pad || framework_failure_
for i in 1 2 3 4 5; do
  touch pad/f$i || framework_failure_
  setfacl -m "u:$(id -u):rw" pad/f$i 2>/dev/null || framework_failure_
done

# The gap between the '+' indicator and the link count must be the same
# whether the listing contains one or several ACL entries.
gap1=$(ls -l pad/f1 | sed -n 's/^[^+]*+\( *\)[0-9].*/\1/p')
gap5=$(ls -l pad    | sed -n 's/^[^+]*+\( *\)[0-9].*/\1/p' | head -n1)
test "x$gap1" = "x$gap5" || fail=1

Exit $fail
