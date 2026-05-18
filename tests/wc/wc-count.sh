#!/bin/sh
# Test some cases where 'wc -c' uses the splice to /dev/null.
# We don't use splice at wc yet. But uutils broke wc by splice.

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
print_ver_ wc
uses_strace_

truncate -s 4097 page_plus_one ||  skip_ "truncate failed"
cat page_plus_one > /dev/null || skip_ "missing /dev/null"

# Ensure we fallback to write when we don't have sink
hide_dev() {
  unshare -rm $SHELL -c 'mount -t tmpfs tmpfs /dev && "$@"' -- "$@"
}
if hide_dev true;then
  cat page_plus_one |  hide_dev wc -c > out || fail=1
  test $(cat out) == 4097 || fail=1
fi

# Ensure we don't write to fake /dev/null
: > fake 
fake_dev() {
  unshare -rm $SHELL -c 'mount --bind fake /dev/null && "$@"' -- "$@"
}
if fake_dev true;then
  cat page_plus_one |  fake_dev wc -c > out || fail=1
  test $(cat out) == 4097 || fail=1
  test -s fake && fail=1
fi

# Ensure we fallback to write() if there is an issue with (async) zero-copy to /dev/null
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
  cat page_plus_one |  no_zero_copy wc -c > out || fail=1
  test $(cat out) == 4097 || fail=1
fi

# Ensure we fallback to write() if there is an issue with pipe2() or we don't use it
# For example if we don't have enough file descriptors available.
no_pipe() { strace -f -o /dev/null -e inject=pipe,pipe2:error=EMFILE "$@"; }
if no_pipe true; then
  no_pipe wc -c > out || fail=1
  test $(cat out) == 4097 || fail=1
fi

Exit $fail
