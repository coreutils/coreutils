#!/bin/sh
# Ignore ENOTSUP only when changing a symlink itself.

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
print_ver_ chmod
require_strace_ chmod
getlimits_

# Match all chmod variants, including fchmodat2 when supported.
inject='/.*chmod:error=EOPNOTSUPP'
strace --quiet=all -o /dev/null -e inject="$inject" true 2>/dev/null \
  || skip_ 'strace does not support the required injection options'

touch file || framework_failure_
ln -s file link || framework_failure_

for deref in --dereference --no-dereference; do
  for file in file link; do
    strace --quiet=all -o trace -e inject="$inject" \
      chmod "$deref" u+w "$file" >out 2>err
    status=$?
    # Only unsupported changes to the symlink itself may be ignored.
    # Emulation may reject symlinks without making a chmod syscall.
    if test "$deref:$file" = --no-dereference:link; then
      test "$status" = 0 || fail=1
      compare /dev/null err || fail=1
    else
      if ! grep ' (INJECTED)' trace > /dev/null; then
        test "$fail" = 0 || break 2
        skip_ 'strace did not inject the requested failure'
      fi
      test "$status" = 1 || fail=1
      echo "chmod: changing permissions of '$file': $ENOTSUP" >exp \
        || framework_failure_
      compare exp err || fail=1
    fi
    compare /dev/null out || fail=1
  done
done

Exit $fail
