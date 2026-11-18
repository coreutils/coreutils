#!/bin/sh
# Ensure tail follows files across truncation and replacement.

# Copyright (C) 2015-2026 Free Software Foundation, Inc.

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
getlimits_

check_tail_output()
{
  local delay="$1"
  grep "$tail_re" out > /dev/null ||
    { sleep $delay; return 1; }
}

# Terminate any background tail process
cleanup_() { kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null; pid=; }

# Speedup the non inotify case
fastpoll='-s.1 --max-unchanged-stats=1'

# Start with fewer than 10 lines. Pick a larger size to ensure
# we test multiple reads.  Note BUFSIZ currently suffices but we
# use IO_BUFSIZE to decouple from the implementation somewhat.
printf '%*s\n10\n' "$(($IO_BUFSIZE * 2))" '' > big || framework_failure_

for follow in '-f' '-F'; do
  for mode in '' '---disable-inotify'; do
    rm -f out
    cp big f || framework_failure_

    tail $follow $mode $fastpoll f > out 2>&1 & pid=$!

    # Wait up to 12.7s for tail to start
    tail_re='^10$' retry_delay_ check_tail_output .1 7 ||
      { cat out; fail=1; }

    # coreutils 9.9-9.12 had a stale read position in the first buffer
    # which hid truncation, and also kept polling tail -F from rechecking
    # the file name, so check that also.
    if test "$follow" = '-f'; then
      truncate -s "$IO_BUFSIZE" f || framework_failure_
    else
      printf '%*s' "$IO_BUFSIZE" '' > new && mv -f new f || framework_failure_
    fi
    printf '\nchanged\n' >> f || framework_failure_
    tail_re='^changed$' retry_delay_ check_tail_output .1 7 ||
      { tail -c 256 out; fail=1; }

    seq 11 15 > f || framework_failure_

    # Wait up to 12.7s for new data
    tail_re='^15$' retry_delay_ check_tail_output .1 7 ||
      { cat out; fail=1; }

    cleanup_
  done
done

Exit $fail
