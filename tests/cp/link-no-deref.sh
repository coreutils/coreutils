#!/bin/sh
# Ensure that cp --link --no-dereference works properly

# Copyright (C) 2006-2026 Free Software Foundation, Inc.

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
print_ver_ cp
getlimits_
uses_strace_

ln -s no-such-file dangling-slink || framework_failure_


# Prior to coreutils-6.0, this would fail on non-Linux kernels,
# with link being applied to the dangling symlink.
cp --link --no-dereference dangling-slink d2 || fail=1

NL='
'
chowns=chown,lchown,fchownat
if strace --quiet=all -o /dev/null -P foo \
          -e inject=$chowns:error=EPERM true 2>/dev/null; then
  echo "cp: failed to preserve ownership for ''\$'\\n': $ENOSYS" >exp \
    || framework_failure_
  touch file && ln -s file slink || framework_failure_
  returns_ 1 strace --quiet=all -o /dev/null -P "$NL" \
    -e inject=$chowns:error=ENOSYS cp -P --preserve=ownership slink "$NL" \
    >out 2>err || fail=1
  compare exp err || fail=1
  compare /dev/null out || fail=1
fi

Exit $fail
