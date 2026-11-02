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
print_ver_ chcon
require_strace_ getxattr

mkdir -p d/foo || framework_failure_
touch d/foo/bar || framework_failure_
opens=open,openat
xattrs=getxattr,lgetxattr,setxattr,lsetxattr

(cd d && chcon -R -t user_tmp_t . > /dev/null 2>&1) \
  || skip_ 'chcon does not work on the test file system'

# Skip if strace does not support path-filtered syscall injection.
strace --quiet=all -o /dev/null -P d/foo -P d/foo/bar \
  -e inject=$opens,$xattrs:error=ENOENT true 2> /dev/null \
  || skip_ 'strace does not support the required options and syscalls'

# Test traversal using a partial context, where we use openat to
# avoid races.
for errnum in ENOENT; do
  rm -f err trace
  (cd d && strace --quiet=all -o ../trace -P foo -P foo/bar \
     -e inject=$opens,$xattrs:error=$errnum chcon -R -t user_tmp_t . \
     > /dev/null 2> ../err)
  status=$?

  grep ' (INJECTED)' trace > /dev/null 2>&1 \
    || skip_ 'strace did not inject the requested failure'
  test $status -eq 0 || fail=1
  test ! -s err || { cat err; fail=1; }
done

# Test traversal on a complete context, where getfileconat
# or lgetfileconat are used.  This requires root, and we use
# root:object_r:tmp_t:s0 since it works without mcstransd running.
if uid_is_privileged_; then
  for errnum in ENOENT; do
    rm -f err trace
    (cd d && strace --quiet=all -o ../trace -P /proc/self/fd/4/foo \
       -P /proc/self/fd/5/bar -e inject=$xattrs:error=$errnum \
       chcon -R root:object_r:tmp_t:s0 . > /dev/null 2> ../err)
    status=$?

    grep ' (INJECTED)' trace > /dev/null 2>&1 \
      || skip_ 'strace did not inject the requested failure'
    test $status -eq 0 || fail=1
    test ! -s err || { cat err; fail=1; }
  done
fi

Exit $fail
