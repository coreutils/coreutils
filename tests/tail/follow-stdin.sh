#!/bin/sh
# Test tail -f -

# Copyright (C) 2009-2025 Free Software Foundation, Inc.

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

#################
# tail -f - would fail with the initial inotify implementation

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

echo line > in || framework_failure_
echo line > exp || framework_failure_

for mode in '' '---disable-inotify'; do
  > out || framework_failure_

  tail $mode -f $fastpoll < in > out 2> err & pid=$!

  # Wait up to 12.7s for output to appear:
  tail_re='line' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: a: unexpected delay?"; cat out; fail=1; }

  # Ensure there was no error output.
  compare /dev/null err || fail=1

  cleanup_
done


#################
# Before coreutils-8.26 this would induce an UMR under UBSAN
returns_ 1 timeout 10 tail -f - <&- 2>errt || fail=1
cat <<\EOF >exp || framework_failure_
tail: cannot fstat 'standard input'
tail: no files remaining
EOF
sed 's/\(tail:.*\):.*/\1/' errt > err || framework_failure_
compare exp err || fail=1


#################
# Before coreutils-8.28 this would erroneously issue a warning
if tty -s </dev/tty && test -t 0; then
  for input in '' '-' '/dev/tty'; do
    returns_ 124 timeout 0.1 tail -f $input 2>err || fail=1
    compare /dev/null err || fail=1
  done

  tail -f - /dev/null </dev/tty 2> out & pid=$!
  # Wait up to 12.7s for output to appear:
  tail_re='ineffective' retry_delay_ check_tail_output .1 7 ||
    { cat out; fail=1; }
  cleanup_
fi

Exit $fail
