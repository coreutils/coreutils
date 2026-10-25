#!/bin/sh
# Test that 'chown --verbose' works properly on dangling symbolic links.

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
print_ver_ chown
getlimits_

ln -s missing dangling-slink || framework_failure_

set _ $(ls -ld dangling-slink); shift; user="$3"; group="$4"

# From coreutils-5.3.0 to coreutils-9.12, this would use an
# uninitialized struct stat.  Commonly, this would lead to the
# following message:
#
#   failed to change ownership of 'dangling-slink' \
#     from root:root to collin:collin
#
# Or, random integers if the st_gid and st_uid members were not
# initialized to zero by the compiler.

echo "chown: cannot dereference 'dangling-slink': $ENOENT" >exp-err \
  || framework_failure_
echo "failed to change ownership of 'dangling-slink' to $user:$group" \
  >exp-out || framework_failure_
returns_ 1 chown -v "$user":"$group" dangling-slink >out 2>err || fail=1
compare exp-out out || fail=1
compare exp-err err || fail=1

Exit $fail
