#!/bin/sh
# Ensure that moving a directory across file systems preserves both
# hard links and symlinks contained in it.

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
print_ver_ mv
cleanup_() { rm -rf "$other_partition_tmpdir"; }
. "$abs_srcdir/tests/other-fs-tmpdir"

# A directory holding a hard linked pair of files, and a symlink to one
# of them.  The link count > 1 combined with the symlink used to be
# enough to confuse some implementations into replacing the regular
# files with symlinks, thus losing the data.
mkdir d || framework_failure_
printf 'important data' > d/realfile || framework_failure_
ln d/realfile d/realfile2 || framework_failure_
ln -s realfile d/link1 || framework_failure_

printf 'important data' > exp || framework_failure_

mv d "$other_partition_tmpdir" || fail=1

# The source directory must be gone.
test -d d && fail=1

dest="$other_partition_tmpdir/d"

# The regular files must still be regular files with the original data.
for f in realfile realfile2; do
  test -f "$dest/$f" || fail=1
  test -L "$dest/$f" && fail=1
  compare exp "$dest/$f" || fail=1
done

# ... and must still be hard linked together.
set -- $(ls -Ci "$dest/realfile" "$dest/realfile2")
test $1 = $3 || fail=1

# The symlink must still be a symlink to realfile.
test -L "$dest/link1" || fail=1
test "$(readlink "$dest/link1")" = realfile || fail=1

# Test symbolic links to "." across file systems.
(mkdir -p "$dest"/a "$dest"/b c &&
  ln -s . "$dest"/a/symlink1 &&
  ln -s . "$dest"/a/symlink2) || framework_failure_
(cd c && timeout -v 10 mv "$dest"/a "$dest"/b/ >out 2>err) || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1
returns_ 1 test -d "$dest"/a || fail=1
test -d "$dest"/b || fail=1
test -d "$dest"/b/a || fail=1
test -L "$dest"/b/a/symlink1 || fail=1
test -L "$dest"/b/a/symlink2 || fail=1
test "$(readlink "$dest"/b/a/symlink1)" = . || fail=1
test "$(readlink "$dest"/b/a/symlink2)" = . || fail=1

Exit $fail
