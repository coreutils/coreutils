#!/bin/sh
# test program group handling

# Copyright (C) 2011-2026 Free Software Foundation, Inc.

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
print_ver_ timeout env
require_trap_signame_
require_kill_group_

# construct a program group hierarchy as follows:
#  timeout-group - foreground group
#    group.sh - separate group
#      timeout.cmd - same group as group.sh
#
# We then send a SIGUSR1 to the "separate group"
# to simulate what happens when a terminating signal
# is sent to the foreground group.

setsid true || skip_ "setsid required to control groups"

printf '%s\n' '#!'"$SHELL" > timeout.cmd || framework_failure_
cat >> timeout.cmd <<\EOF
trap 'touch sig.received; exit' USR1
trap
touch timeout.running
count=$1
until test -e sig.received || test $count = 0; do
  sleep 1
  count=$(expr $count - 1)
done
EOF
chmod a+x timeout.cmd

cat > group.sh <<EOF
#!$SHELL

# trap '' ensures this script ignores the signal,
# so that the 'wait' below is not interrupted.
# Note this then requires env --default... to reset
# the signal disposition so that 'timeout' handles it.
# Alternatively one could use trap ':' USR1
# and then handle the retry in wait like:
# while wait; test \$? -gt 128; do :; done
# Note also INT and QUIT signals are special for backgrounded
# processes like this in shell as they're auto ignored
# and can't be reset with trap to any other disposition.
# Therefore we use the ignored signal method so any
# termination signal can be used.
trap '' USR1

env --default-signal=USR1 \
timeout -v --foreground 25 ./timeout.cmd 20&
wait
echo group.sh wait returned \$ret
EOF
chmod a+x group.sh

check_timeout_cmd_running()
{
  local delay="$1"
  test -e timeout.running ||
    { sleep $delay; return 1; }
}

check_timeout_cmd_exiting()
{
  local delay="$1"
  test -e sig.received ||
    { sleep $delay; return 1; }
}

# Terminate any background processes
cleanup_() { kill $pid 2>/dev/null && wait $pid; }

# Start above script in its own group.
# We could use timeout for this, but that assumes an implementation.
setsid ./group.sh & pid=$!
# Wait 6.3s for timeout.cmd to start
retry_delay_ check_timeout_cmd_running .1 6 || fail=1
# Simulate a Ctrl-C to the group to test timely exit
kill -USR1 -- -$pid
wait
test -e sig.received || fail=1
rm -f sig.received timeout.running

# On Linux ensure we kill the monitored command
# even if we're terminated abnormally (e.g., get SIGKILL).
if grep '^#define HAVE_PRCTL 1' "$CONFIG_HEADER" >/dev/null; then
  timeout -sUSR1 25 ./timeout.cmd 20 & pid=$!
  # Wait 6.3s for timeout.cmd to start
  retry_delay_ check_timeout_cmd_running .1 6 || fail=1
  kill -KILL -- $pid
  wait
  # Wait 6.3s for timeout.cmd to exit
  retry_delay_ check_timeout_cmd_exiting .1 6 || fail=1
  rm -f sig.received timeout.running
fi

# Ensure cascaded timeouts work
# or more generally, ensure we timeout
# commands that create their own group
# This didn't work before 8.13.

start=$(date +%s)

# Note the first timeout must send a signal that
# the second is handling for it to be propagated to the command.
# termination signals are implicitly handled unless ignored.
timeout -sALRM 30 timeout -sUSR1 25 ./timeout.cmd 20 & pid=$!
# Wait 6.3s for timeout.cmd to start
retry_delay_ check_timeout_cmd_running .1 6 || fail=1
kill -ALRM $pid # trigger the alarm of the first timeout command
wait $pid
ret=$?
test $ret -eq 124 ||
  skip_ "timeout returned $ret. SIGALRM not handled?"
test -e sig.received || fail=1

end=$(date +%s)

test $(expr $end - $start) -lt 20 ||
  skip_ "timeout.cmd didn't receive a signal until after sleep?"

Exit $fail
