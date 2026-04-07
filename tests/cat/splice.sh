#!/bin/sh
# Test some cases where 'cat' uses the splice system call.

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

# Check the non pipe output case, since that is different with splice
if timeout 10 true; then
  timeout .1 cat /dev/zero >/dev/null
  test $? = 124 || fail=1
fi

# Verify splice is called multiple times,
# and doesn't just fall back after the first EINVAL.
# This also implicitly handles systems without splice at all.
strace -o splice_count -e splice cat /dev/zero | head -c1M >/dev/null
splice_count=$(grep '^splice' splice_count | wc -l)

# Test that splice errors are diagnosed.
# Odd numbers are for input, even for output
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

# Ensure we fallback to write() if there is an issue with (async) zero-copy
zc_syscalls='io_uring_setup io_uring_enter io_uring_register memfd_create
             sendfile splice tee vmsplice'
syscalls=$(
  for s in $zc_syscalls; do
    strace -qe "$s" true >/dev/null 2>&1 && echo "$s"
  done | paste -s -d,)

no_zero_copy() {
  strace -f -o /dev/null -e inject=${syscalls}:error=ENOSYS "$@"
}
if no_zero_copy true; then
  test "$(no_zero_copy cat /dev/zero | head -c 2 | tr '\0' 'y')" = 'yy' \
    || fail=1
fi
# Ensure we fallback to write() if there is an issue with pipe2()
# For example if we don't have enough file descriptors available.
no_pipe() { strace -f -o /dev/null -e inject=pipe,pipe2:error=EMFILE "$@"; }
if no_pipe true; then
  no_pipe timeout .1 cat /dev/zero >/dev/null
  test $? = 124 || fail=1
fi

Exit $fail
