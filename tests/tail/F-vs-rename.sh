#!/bin/sh
# Demonstrate that tail -F works when renaming the tailed files.
# Between coreutils 7.5 and 8.2 inclusive, 'tail -F a b' would
# stop tracking additions to b after 'mv a b'.

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

check_tail_output() {
  local delay="$1"
  if [ "$tail_re" ]; then
    grep "$tail_re" err ||
      { sleep $delay; return 1; }
  else
    local tail_re="==> $file <==@$data@"
    tr '\n' @ < out | grep "$tail_re" > /dev/null ||
      { sleep $delay; return 1; }
  fi
  unset tail_re
}

# Terminate any background tail process
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'

for mode in '' '---disable-inotify'; do
  rm -f a b out err
  touch a b || framework_failure_

  tail $mode -F $fastpoll a b >out 2>err & pid=$!

  # Wait up to 12.7s for tail to start.
  echo x > a
  file=a data=x retry_delay_ check_tail_output .1 7 || { cat out; fail=1; }

  mv a b || framework_failure_

  # Wait 12.7s for this diagnostic:
  # tail: 'a' has become inaccessible: No such file or directory
  tail_re='inaccessible' retry_delay_ check_tail_output .1 7 ||
    { cat out; fail=1; }

  # Wait 12.7s for this diagnostic:
  # tail: 'b' has been replaced;  following new file
  tail_re='replaced' retry_delay_ check_tail_output .1 7 ||
    { cat out; fail=1; }
  # Wait up to 12.7s for "x" to be displayed:
  file='b' data='x' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: b: unexpected delay 1?"; cat out; fail=1; }

  echo x2 > a
  # Wait up to 12.7s for this to appear in the output:
  # "tail: '...' has appeared;  following new file"
  tail_re='has appeared' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: a: unexpected delay 2?"; cat out; fail=1; }
  # Wait up to 12.7s for "x2" to be displayed:
  file='a' data='x2' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: a: unexpected delay 3?"; cat out; fail=1; }

  echo y >> b
  # Wait up to 12.7s for "y" to appear in the output:
  file='b' data='y' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: b: unexpected delay 4?"; cat out; fail=1; }

  echo z >> a
  # Wait up to 12.7s for "z" to appear in the output:
  file='a' data='z' retry_delay_ check_tail_output .1 7 ||
    { echo "$0: a: unexpected delay 5?"; cat out; fail=1; }

  cleanup_
done

Exit $fail
