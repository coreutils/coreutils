#!/bin/sh
# Ensure that "tail --pid=PID fifo" exits responsively

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

mkfifo_or_skip_ fifo


# Terminate any background process
cleanup_()
{
  for p in $pid $writer_pid; do
    kill $p 2>/dev/null
  done
  for p in $pid $writer_pid; do
    wait $p 2>/dev/null
  done

  pid=
  writer_pid=
}

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'


# Ensure an absent FIFO writer doesn't block tail from checking --pid.
sleep 1 & pid=$!
returns_ 124 timeout 10 tail -f $fastpoll --pid=$pid fifo && fail=1
cleanup_


# Ensure a silent FIFO writer doesn't block tail from checking --pid.
rm -f writer-ready || framework_failure_

writer_ready_()
{
  sleep $1
  test -e writer-ready
}

# Simulate a writer that may wait a long time between writes
silent_writer() {
  exec 3>fifo &&
  touch writer-ready &&
  exec sleep 20
}
silent_writer & writer_pid=$!

# allow fifo to open
timeout 10 $SHELL -c ': < fifo' || framework_failure_
retry_delay_ writer_ready_ .1 6 || framework_failure_

sleep 1 & pid=$!
returns_ 124 timeout 10 tail -f $fastpoll --pid=$pid fifo && fail=1
cleanup_


Exit $fail
