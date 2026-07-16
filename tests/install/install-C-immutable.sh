#!/bin/sh
# Ensure "install -C" does not attempt to chown/chmod an unchanged,
# immutable destination file.
# Requires root access to do chattr +i, as well as a file system
# that supports the immutable attribute (e.g., ext2/3/4, xfs, btrfs).

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
print_ver_ ginstall
require_root_

# These simple one-file operations are expected to work even in the
# presence of this bug, and we need them to set up the rest of the test.
chattr_i_works=1
touch f || framework_failure_
chattr +i f 2>/dev/null || chattr_i_works=0
rm f 2>/dev/null
test -f f || chattr_i_works=0
chattr -i f 2>/dev/null || chattr_i_works=0
rm f 2>/dev/null || chattr_i_works=0
test -f f && chattr_i_works=0

if test $chattr_i_works = 0; then
  skip_ "chattr +i doesn't work on this file system"
fi

u1=1
g1=1

echo test > a || framework_failure_

cleanup_() { chattr -i b; }

# Install once to create the destination, then make it immutable.
ginstall -Cv -m0644 -o$u1 -g$g1 a b || framework_failure_
chattr +i b || framework_failure_

# A second "-C" install with an identical source, mode, owner and group
# must be a no-op: it must not attempt to chown/chmod the destination,
# since that would fail on an immutable file even though nothing about
# it would actually change.
ginstall -Cv -m0644 -o$u1 -g$g1 a b || fail=1

Exit $fail
