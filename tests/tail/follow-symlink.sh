#!/bin/sh
# Test that --follow works on a symbolic link.

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

check_tail_output()
{
  local delay="$1"
  grep "$tail_re" out ||
    { sleep $delay; return 1; }
}

# Terminate any background tail process
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'

for mode in '' '---disable-inotify'; do
  rm -f file link out || framework_failure_
  touch file || framework_failure_
  ln -s file link || framework_failure_
  tail $mode $fastpoll -f link >out 2>err & pid=$!
  for c in a b c; do
    # Wait up to 12.7s for tail to start.
    echo "$c" >> file
    tail_re="^$c$" retry_delay_ check_tail_output .1 7 || { cat out; fail=1; }
  done
  compare file out || fail=1
  compare /dev/null err || fail=1
  cleanup_
done

Exit $fail
