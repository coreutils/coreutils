#!/bin/sh
# Test --debug output

# Copyright (C) 2025-2026 Free Software Foundation, Inc.

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
print_ver_ tail

# Terminate any background tail process
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

cleanup_fail_ ()
{
  warn_ $1
  cleanup_
  fail=1
}

# $check_re - string to be found
# $check_f - file to be searched
check_tail_output_ ()
{
  local delay="$1"
  grep "$check_re" $check_f > /dev/null ||
    { sleep $delay ; return 1; }
}

grep_timeout_ ()
{
  check_re="$1"
  check_f="$2"
  retry_delay_ check_tail_output_ .1 5
}


timeout 10 tail --debug -f /dev/null 2>debug.out & pid=$!
grep_timeout_ 'tail: using blocking mode' 'debug.out' || fail=1
cleanup_

timeout 10 tail --debug -F /dev/null 2>debug.out & pid=$!
grep_timeout_ 'tail: using polling mode' 'debug.out' || fail=1
cleanup_

touch file.debug
require_strace_ 'inotify_add_watch'
returns_ 124 timeout .1 strace -e inotify_add_watch -o strace.out \
  tail -F file.debug || fail=1
if grep 'inotify' strace.out; then
  timeout 10 tail --debug -n0 -f file.debug 2>debug.out & pid=$!
  grep_timeout_ 'tail: using notification mode' 'debug.out' || fail=1
  cleanup_

  timeout 10 tail --debug ---disable-inotify -f file.debug 2>debug.out & pid=$!
  grep_timeout_ 'tail: using polling mode' 'debug.out' || fail=1
  cleanup_
fi

Exit $fail
