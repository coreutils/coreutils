#!/bin/sh
# Verify chmod symlink handling options

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
print_ver_ chmod

#dirs
mkdir -p a/b a/c || framework_failure_
#files
touch a/b/file a/c/file || framework_failure_
#dangling link
ln -s foo a/dangle || framework_failure_
#link to file
ln -s ../b/file a/c/link || framework_failure_
#link to dir
ln -s b a/dirlink || framework_failure_

# tree -F a
#  a/
#  |-- b/
#  |   '-- file
#  |-- c/
#  |   |-- file
#  |   '-- link -> ../b/file
#  |-- dangle -> foo
#  '-- dirlink -> b/

reset_modes() { chmod =777 a/b a/c a/b/file a/c/file || fail=1; }
count_755() {
  test "$(grep 'rwxr-xr-x' 'out' | wc -l)" = "$1" || { cat out; fail=1; }
}

reset_modes
# -R (with default -H) does not deref traversed symlinks (only cli args)
chmod 755 -R a/c || fail=1
ls -ld a/c a/c/file a/b/file > out || framework_failure_
count_755 2

reset_modes
# set a/c a/c/file and a/b/file (through symlink) to 755
chmod 755 -LR a/c || fail=1
ls -ld a/c a/c/file a/b/file > out || framework_failure_
count_755 3

reset_modes
# do not set /a/b/file through symlink (should try to chmod the link itself)
chmod 755 -RP a/c/ || fail=1
ls -l a/b > out || framework_failure_
count_755 0

reset_modes
# set /a/b/file through symlink
chmod 755 --dereference a/c/link || fail=1
ls -l a/b > out || framework_failure_
count_755 1

reset_modes
# do not set /a/b/file through symlink (should try to chmod the link itself)
chmod 755 --no-dereference a/c/link 2>err || fail=1
ls -l a/b > out || framework_failure_
count_755 0

# Dangling links should not induce an error if not dereferencing
for noderef in '-h' '-RP' '-P'; do
  chmod 755 --no-dereference $noderef a/dangle 2>err || fail=1
done
# Dangling links should induce an error if dereferencing
for deref in '' '--deref' '-R'; do
  returns_ 1 chmod 755 $deref a/dangle 2>err || fail=1
done

Exit $fail
