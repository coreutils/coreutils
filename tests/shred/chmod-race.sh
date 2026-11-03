#!/bin/sh
# Test the parent directory race in shred --force.

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
print_ver_ shred timeout
require_strace_ openat

mkdir d1 d2 &&
echo shred >d1/f &&
echo saved >d2/f &&
cp d2/f exp &&
chmod 400 d1/f d2/f || framework_failure_

# Stop after the failed open, before chmod can resolve the pathname.
# Inject EACCES so the test also works when run as root.
# Note we wrap with timeout as strace ignores SIGTERM with -o.
# Also even with --interruptible=wait strace might not handle
# stopped tracee's appropriately.
inject=open,openat:error=EACCES:signal=SIGSTOP:when=1
timeout -k 1 60 strace --quiet=all -o /dev/null -P d1/f -P f \
  -e inject=$inject true 2>/dev/null \
  || skip_ 'strace does not support the required injection options'

tracer=
cleanup_()
{
  test ! -s pid || kill -KILL "$(cat pid)" 2>/dev/null
  if test -n "$tracer"; then
    kill "$tracer" 2>/dev/null
    wait "$tracer" 2>/dev/null
  fi
}

timeout -k 1 60 strace --quiet=all -o trace -P d1/f -P f -e inject=$inject \
  $SHELL -c 'echo $$ >pid; exec shred -f -n0 -z d1/f' 2>err & tracer=$!
stopped_() { sleep "$1"; grep 'stopped by SIGSTOP' trace >/dev/null; }
retry_delay_ stopped_ .1 6 || framework_failure_ 'shred did not stop'

mv d1 d1.orig && ln -s d2 d1 || framework_failure_
kill -CONT "$(cat pid)" || framework_failure_
wait "$tracer" || fail=1
tracer=
rm -f pid

test "$(stat -c %a d1.orig/f)" = 200 || fail=1
test "$(stat -c %a d2/f)" = 400 || fail=1
compare exp d2/f || fail=1
compare /dev/null err || fail=1

Exit $fail
