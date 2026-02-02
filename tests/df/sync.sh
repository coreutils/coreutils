#!/bin/sh
# Ensure 'df --sync' works.

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
print_ver_ df

# This test is marked "very expensive" since calling sync() can take a long
# time on a busy system.
very_expensive_
require_strace_ 'sync,statfs,fstatfs'

# Check that sync was called before statfs or statvfs.
check_sync ()
{
  seen_sync=0
  while IFS= read line; do
    case "$line" in
      sync\(*) seen_sync=1 ;;
      statfs\(*|fstatfs\(*)
        if test "$seen_sync" -eq 1; then
          return 0
        else
          return 1
        fi
        ;;
    esac
  done < "$1"
  # Fail if we don't see statfs or fstatfs.
  return 1
}

# Make sure that 'df --sync' calls sync() before gathering usage information.
strace -o strace.out -e trace=sync,statfs,fstatfs df --sync || fail=1
check_sync strace.out || fail=1

# Also check it with a file given as the argument.
strace -o strace.out -e trace=sync,statfs,fstatfs df --sync . || fail=1
check_sync strace.out || fail=1

Exit $fail
