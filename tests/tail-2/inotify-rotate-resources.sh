#!/bin/sh
# ensure that tail -F doesn't leak inotify resources

# Copyright (C) 2015 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. "${srcdir=.}/tests/init.sh"; path_prepend_ ./src
print_ver_ tail

grep '^#define HAVE_INOTIFY 1' "$CONFIG_HEADER" >/dev/null \
  || skip_ 'inotify required'

require_strace_ inotify_rm_watch

check_tail_output()
{
  local delay="$1"
  grep "$tail_re" out > /dev/null ||
    { sleep $delay; return 1; }
}

# Wait up to 25.5 seconds for grep REGEXP 'out' to succeed.
grep_timeout() { tail_re="$1" retry_delay_ check_tail_output .1 8; }

check_strace()
{
  local delay="$1"
  grep "$strace_re" strace.out > /dev/null ||
    { sleep $delay; return 1; }
}

cleanup_fail()
{
  cat out
  warn_ $1
  fail=1
}

fastpoll='-s.1 --max-unchanged-stats=1'

# Normally less than a second is required here, but with heavy load
# and a lot of disk activity, even 20 seconds per grep_timeout is insufficient,
# which leads to this timeout (used as a safety net for cleanup)
# killing tail before processing is completed.
touch k || framework_failure_
timeout 180 strace -e inotify_rm_watch -o strace.out \
  tail -F $fastpoll k >> out 2>&1 & pid=$!

reverted_to_polling_=0
for i in $(seq 2); do
    echo $i

    echo 'tailed' > k;
    # wait for 'tailed' in (after first iteration; new) file and then in 'out'
    grep_timeout 'tailed' || { cleanup_fail 'failed to find "tailed"'; break; }

    mv k k.tmp
    # wait for tail to detect the rename
    grep_timeout 'inaccessible' ||
      { cleanup_fail 'failed to detect rename'; break; }

    # Assume this is not because we're leaking.
    # The explicit check for inotify_rm_watch should confirm that.
    grep -F 'reverting to polling' out >/dev/null &&
      { reverted_to_polling_=1; break; }

    # Note we strace here rather than consuming all available watches
    # to be more efficient, but more importantly avoid depleting resources.
    # Note also available resources can currently be tuned with:
    #  sudo sysctl -w fs.inotify.max_user_watches=$smallish_number
    # However that impacts all processes for the current user, and also
    # may not be supported in future, instead being auto scaled to RAM
    # like the Linux epoll resources were.
    if test "$i" -gt 1; then
      strace_re='inotify_rm_watch' retry_delay_ check_strace .1 8 ||
        { cleanup_fail 'failed to find inotify_rm_watch syscall'; break; }
    fi

    >out && >strace.out || framework_failure_ 'failed to reset output files'
done

test "$reverted_to_polling_" = 1 && skip_ 'inotify resources already depleted'
kill $pid
wait $pid

Exit $fail
