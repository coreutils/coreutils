#!/bin/sh
# Test that read/write errors are distinguished

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
print_ver_ cat
getlimits_
uses_strace_

# write error
if test -w /dev/full && test -c /dev/full; then
  echo "cat: write error: $ENOSPC" >exp || framework_failure_
  echo 1 | returns_ 1 cat >/dev/full 2>err || fail=1
  compare exp err || fail=1
fi

# read error
errf='/proc/self/mem'
if returns_ 1 dd if=$errf bs=1 count=1 status=none 2>errt; then
  err_mem=$(sed 's/dd: .*: //' <errt) || framework_failure_
  echo "cat: $errf: $err_mem" >exp || framework_failure_
  returns_ 1 cat $errf 2>err || fail=1
  compare exp err || fail=1
fi

# Verify splice is called multiple times,
# and doesn't just fall back after the first EINVAL.
# This also implicitly handles systems without splice at all.
strace -o splice_count -e splice cat /dev/zero | head -c1M >/dev/null
splice_count=$(grep '^splice' splice_count | wc -l)

# Test that splice errors are diagnosed.
# Odd numbers are for input, even for output (since cat is not buffered)
if test "$splice_count" -gt 1 &&
   strace -o /dev/null -e inject=splice:error=EIO:when=3 true; then
  for when in 3 4; do
    test "$when" = 4 && efile='write error' || efile='/dev/zero'
    printf 'cat: %s: %s\n' "$efile" "$EIO" > exp || framework_failure_
    returns_ 1 timeout 10 strace -o /dev/null \
      -e inject=splice:error=EIO:when=$when \
      cat /dev/zero >/dev/null 2>err || fail=1
    compare exp err || fail=1
  done
fi

Exit $fail
