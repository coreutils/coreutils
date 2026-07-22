#!/bin/sh
# Test ignoring traversal races using strace fault injection.

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
print_ver_ ls chmod chown chgrp du
require_strace_ stat

mkdir d || framework_failure_
touch d/foo || framework_failure_
uid=$(id -u) || framework_failure_
gid=$(id -g) || framework_failure_

stats=$(get_stat_syscalls_)
chmods=chmod,fchmodat,fchmodat2
chowns=chown,lchown,fchownat

# Skip if strace does not support path-filtered syscall injection.
strace --quiet=all -o /dev/null -P foo \
  -e inject=$stats:error=ENOENT true 2> /dev/null \
  || skip_ 'strace does not support the required options and syscalls'

run_injected_ ()
{
  path=$1
  syscalls=$2
  shift 2
  rm -f err trace
  (cd d && strace --quiet=all -o ../trace -P "$path" \
     -e inject="$syscalls:error=$errnum" "$@" > /dev/null 2> ../err)
  status=$?

  grep ' (INJECTED)' trace > /dev/null 2>&1 \
    || skip_ 'strace did not inject the requested failure'
  test $status -eq $expected_status || fail=1
  if test $expected_status -eq 0; then
    test ! -s err || { cat err; fail=1; }
  else
    test -s err || fail=1
  fi
}

expected_status=0
for errnum in ENOENT EIO; do
  test $errnum = EIO && expected_status=1
  run_injected_ foo "$stats" ls -l .
  run_injected_ foo "$stats" du -a .
  run_injected_ foo "$chmods" chmod -R 0700 .
  run_injected_ foo "$chowns" chown -R "$uid" .
  run_injected_ foo "$chowns" chgrp -R "$gid" .
done

Exit $fail
