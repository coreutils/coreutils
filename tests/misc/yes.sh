#!/bin/sh
# Validate yes buffer handling

# Copyright (C) 2015-2026 Free Software Foundation, Inc.

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
print_ver_ yes
getlimits_
uses_strace_

# Check basic operation
test "$(yes | head -n1)" = 'y' || fail=1

# Check various single item sizes, with the most important
# size being BUFSIZ used for the local buffer to yes(1).
# Note a \n is added, so actual sizes required internally
# are 1 more than the size used here.
for size in 1 1999 4095 4096 8191 8192 16383 16384; do
  printf "%${size}s\n" '' > out.1
  yes "$(printf %${size}s '')" | head -n2 | uniq > out.2
  compare out.1 out.2 || fail=1
done

# Check the many small items case,
# both fitting and overflowing the internal buffer.
# First check that 4000 arguments supported.
if test 4000 -eq $(sh -c 'echo $#' 0 $(seq 4000)); then
  for i in 100 4000; do
    seq $i | paste -s -d ' ' | sed p > out.1
    yes $(seq $i) | head -n2 > out.2
    compare out.1 out.2 || fail=1
  done
fi

# Check a single appropriate diagnostic is output on write error
if test -w /dev/full && test -c /dev/full; then
  # The single output diagnostic expected,
  # (without the possibly varying :strerror(ENOSPC) suffix).
  printf '%s\n' "yes: standard output: $ENOSPC" > exp

  for size in 1 16384; do
    returns_ 1 yes "$(printf %${size}s '')" >/dev/full 2>err || fail=1
    compare exp err || fail=1
  done
fi

# Check the non pipe output case, since that is different with splice
if timeout 10 true; then
  timeout .1 yes >/dev/null
  test $? = 124 || fail=1
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
  test "$(no_zero_copy yes | head -n2 | paste -s -d '')" = 'yy' || fail=1
fi
# Ensure we fallback to write() if there is an issue with pipe2()
# For example if we don't have enough file descriptors available.
no_pipe() { strace -f -o /dev/null -e inject=pipe,pipe2:error=EMFILE "$@"; }
if no_pipe true; then
  no_pipe timeout .1 yes >/dev/null
  test $? = 124 || fail=1
fi

Exit $fail
