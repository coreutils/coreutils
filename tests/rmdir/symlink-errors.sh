#!/bin/sh
# make sure rmdir outputs clear errors in the presence of symlinks

# Copyright (C) 2021-2025 Free Software Foundation, Inc.

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
print_ver_ rmdir

mkdir dir || framework_failure_
ln -s dir sl || framework_failure_
ln -s missing dl || framework_failure_
touch file || framework_failure_
ln -s file fl || framework_failure_

# Ensure a we maintain ENOTDIR so that we provide
# accurate errors on systems on which rmdir(2) does following the symlink/
returns_ 1 rmdir fl/ 2> err || fail=1
# Ensure we diagnose symlink behavior.
printf '%s\n' "rmdir: failed to remove 'fl/': Not a directory" > exp
compare exp err || fail=1

# Also ensure accurate errors from rmdir -p when traversing symlinks
# Up to and including 8.32 rmdir would fail like:
#   rmdir: failed to remove directory 'sl': Not a directory
mkdir dir/dir2 || framework_failure_
returns_ 1 rmdir -p sl/dir2 2> err || fail=1
# Ensure we diagnose symlink behavior.
printf '%s\n' "rmdir: failed to remove 'sl': Not a directory" > exp
compare exp err || fail=1

# Only perform the following on systems that don't follow the symlink
if ! rmdir sl/ 2>/dev/null; then
  # Up to and including 8.32 rmdir would fail like:
  #   rmdir: failed to remove 'sl/': Not a directory
  # That's inconsistent though as rm sl/ gives:
  #   rm: cannot remove 'sl/': Is a directory
  # Also this is inconsistent with other systems
  # which do follow the symlink and rmdir the target.

  new_error="rmdir: failed to remove '%s': Symbolic link not followed\\n"

  # Ensure we diagnose symlink behavior.
  returns_ 1 rmdir sl/ 2> err || fail=1
  printf "$new_error" 'sl/' > exp || framework_failure_
  compare exp err || fail=1

  # Ensure a consistent diagnosis for dangling symlinks etc.
  returns_ 1 rmdir dl/ 2> err || fail=1
  printf "$new_error" 'dl/' > exp || framework_failure_
  compare exp err || fail=1
fi

Exit $fail
