#!/bin/sh
# Test that 'shred' behaves correctly on FIFOs.

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
print_ver_ shred
getlimits_
uses_strace_

stats='stat'
# List of other _file name_ stat functions to increase coverage.
other_stats='statx lstat stat64 lstat64 newfstatat fstatat64'
for stat in $other_stats; do
  strace -qe "$stat" true > /dev/null 2>&1 &&
    stats="$stats,$stat"
done

open_stat_fail ()
{
  strace --quiet=all -o /dev/null -P file -e inject=open,openat:error=ENXIO \
    -e inject=$stats:error=ENOSYS "$@"
}

# If open fails with ENXIO and the subsequent stat fails, e.g., because the
# FIFO was removed or it is a character or block special associated with a
# non-existent device, we should use the error number from open.
if open_stat_fail true; then
  cat <<EOF >exp-err || framework_failure_
shred: file: failed to open for writing: $ENXIO
EOF
  returns_ 1 open_stat_fail shred file >out 2>err || fail=1
  compare exp-err err || fail=1
  compare /dev/null out || fail=1
fi

mkfifo_or_skip_ fifo

# Test 'shred' on a FIFO with no readers.
cat <<\EOF >exp || framework_failure_
shred: fifo: invalid file type
EOF
returns_ 1 timeout 10 shred fifo >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp err || fail=1

# Terminate any background processes.
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

writer_ready_()
{
  sleep $1
  test -e writer-ready
}

fifo_writer_()
{
  exec 3>fifo &&
  touch writer-ready &&
  exec sleep 20
}

# Test 'shred' on a FIFO with a reader.  Use a temporary writer to
# prove that 'cat' opened the FIFO before 'shred' runs, and keep that
# writer open so 'cat' blocks until we close it.
rm -f writer-ready || framework_failure_
timeout 10 cat fifo > /dev/null & pid=$!
fifo_writer_ & writer_pid=$!
retry_delay_ writer_ready_ .1 6 || framework_failure_
returns_ 1 timeout 10 shred fifo >out 2>err || fail=1
compare /dev/null out || fail=1
compare exp err || fail=1
kill $writer_pid 2>/dev/null || framework_failure_
wait $writer_pid 2>/dev/null
writer_pid=
wait $pid || fail=1  # on cat timeout
pid=

Exit $fail
