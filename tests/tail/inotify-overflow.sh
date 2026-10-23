#!/bin/sh
# Ensure tail -F copes with the kernel discarding inotify events

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
print_ver_ tail
require_inotify_supported_

max_events=$(cat /proc/sys/fs/inotify/max_queued_events) &&
test "$max_events" -le 100000 ||
skip_ 'inotify queue limit unknown or too large'

# Terminate any background tail process
cleanup_()
{
  kill -CONT "$pid" 2>/dev/null
  kill "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
  pid=
}

# $check_re - string to be found
check_tail_output_ ()
{
  local delay="$1"
  grep "$check_re" out > /dev/null ||
    { sleep $delay ; return 1; }
}

grep_timeout_ ()
{
  check_re="$1"
  retry_delay_ check_tail_output_ .1 7
}

# Speed up the fallback-to-polling phase.
fastpoll='-s.1 --max-unchanged-stats=1'

mkdir dir && echo x > dir/file || framework_failure_

tail --debug --pid=$$ -F $fastpoll dir/file >out 2>&1 & pid=$!
grep_timeout_ 'using notification mode' ||
  { cat out; skip_ "inotify can't be used"; }

# Fill the event queue while tail is not reading it,
# so that the events for the following rotation are discarded.
kill -STOP $pid || framework_failure_
(cd dir && seq $max_events | xargs touch) || framework_failure_
mv dir/file dir/file.1 &&
echo y > dir/file &&
kill -CONT $pid || framework_failure_

# From coreutils-7.5 to 9.12 inclusive, the new file never noticed.
grep_timeout_ '^y$' || { cat out; fail=1; }
grep 'inotify event queue overflowed' out || {
  cat out
  test "$fail" = 1 || framework_failure_
}


Exit $fail
