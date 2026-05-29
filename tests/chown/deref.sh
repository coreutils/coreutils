#!/bin/sh
# For coreutils-5.2.1 and earlier, chown --dereference would skip
# symlinks having owner/group matching the specified owner/group.

# Copyright (C) 2004-2026 Free Software Foundation, Inc.

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
print_ver_ chown

ln -s no-such dangle || framework_failure_


set _ $(ls -ldo dangle); shift; user=$3

# With 5.2.1 and earlier, this command would mistakenly succeed.
chown --dereference $user dangle 2> out1 && fail=1
sed 's/: [^:]*$//' out1 > out

cat <<\EOF > exp || framework_failure_
chown: cannot dereference 'dangle'
EOF

compare exp out || fail=1

# A symlink pointing back to an ancestor forms a cycle.  With -L, chown
# follows symlinks while recursing, so it must detect the cycle and stop
# rather than descend into it forever.
mkdir -p cyc/b/c || framework_failure_
ln -s "$(pwd)/cyc" cyc/b/c/d || framework_failure_
chown -vRL $user cyc > out2 2>&1 || fail=1
# The symlinked directory is visited exactly once...
grep -F "'cyc/b/c/d'" out2 > /dev/null || { cat out2; fail=1; }
# ...and recursion does not re-enter it through the cycle.
grep -F "'cyc/b/c/d/b'" out2 > /dev/null && { cat out2; fail=1; }

Exit $fail
