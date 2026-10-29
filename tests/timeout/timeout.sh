#!/bin/sh
# Validate timeout basic operation

# Copyright (C) 2008-2026 Free Software Foundation, Inc.

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

# no timeout
timeout 10 true || fail=1

# no timeout (suffix check)
timeout 1d true || fail=1

# disabled timeout
timeout 0 true || fail=1

# exit status propagation
returns_ 2 timeout 10 $SHELL -c 'exit 2' || fail=1

# timeout
returns_ 124 timeout .1 sleep 10 || fail=1

# exit status propagation even on timeout
# exit status should be 128+TERM
returns_ 124 timeout --preserve-status .1 sleep 10 && fail=1

# kill delay. Note once the initial timeout triggers,
# the exit status will be 124 even if the command
# exits on its own accord.
# exit status should be 128+KILL
returns_ 124 timeout -s0 -k1 .1 sleep 10 && fail=1
# Ensure a consistent exit status with --foreground
returns_ 124 timeout --foreground -s0 -k1 .1 sleep 10 && fail=1

# Keep handling SIGALRM after sending it to the process group.
# Also use a tiny kill delay to check that its SIGALRM is not discarded.
for delay in .1 1e-9; do
  returns_ 137 timeout -s ALRM -k $delay .1 \
    env --ignore-signal=ALRM sleep 10 || fail=1
done

# Keep handling SIGCHLD so the command's exit wakes the monitor.
returns_ 124 timeout -s KILL 10 timeout -s CHLD .1 sleep .5 || fail=1

# Ensure 'timeout' is immune to parent's SIGCHLD handler
# Use a subshell and an exec to work around a bug in FreeBSD 5.0 /bin/sh.
(
  trap '' CHLD

  exec timeout 10 true
) || fail=1

# Don't be confused when starting off with a child (Bug#9098).
# Use setsid to avoid sleep being in the test's process group, as
# upon reparenting it can trigger an orphaned process group SIGHUP
# (if there were stopped processes like in tests/tail/overlay-headers.sh).
if setsid true; then
  out=$(setsid sleep .1 & exec timeout .5 $SHELL -c 'sleep 2; echo foo')
  status=$?
  test "$out" = "" && test $status = 124 || fail=1
fi

# Verify --verbose output
cat > exp <<\EOF
timeout: sending signal 0 to command 'sleep'
timeout: sending signal KILL to command 'sleep'
EOF
for opt in -v --verbose; do
  timeout --foreground $opt -s0 -k .1 .1 sleep 10 2> errt
  sed '/^Killed/d' < errt > err || framework_failure_
  compare exp err || fail=1
done

# Ensure we propagate all terminating signals.
# Specifically here we're testing that SIGPIPE is handled.
# I.e., that we're not killed by the SIGPIPE (and leave the sleep running).
# timeout would exit with 141 usually if SIGPIPE wasn't being handled.
echo 124 > timeout.exp || framework_failure_
for opt in '' '--foreground'; do
  { timeout -s KILL 10 timeout $opt -v .1 sleep 10 2>&1
    echo $? >timeout.status; } | :
  compare timeout.exp timeout.status || fail=1
done
# Ensure we don't catch/propagate ignored signals
(trap '' PIPE && timeout 10 yes |:) 2>&1 |
  grep 'Broken pipe' >/dev/null || fail=1

# A diagnostic exceeding the file-size limit must not loop on SIGXFSZ.
if (trap - XFSZ && ulimit -f 0) 2>/dev/null; then
  returns_ 124 timeout -s KILL 10 $SHELL -c '
    trap - XFSZ
    ulimit -f 0 || exit 1
    exec timeout --foreground -v .1 sleep 10 2>err
  ' || fail=1
fi

# Signal the command and its group before writing to a blocked stderr.
file_seen() { sleep "$1"; test -s "$file"; }
file=term.seen
timeout -v .1 $SHELL -c '
  exec 2>/dev/null  # ensure no "Terminated" message blocks the trap
  trap "echo 1 >term.seen" TERM
  yes  # Fill the pipe
' 2>&1 |
  { retry_delay_ file_seen .1 6 || touch term.missed; }
test -e term.missed && fail=1

# A group-wide SIGKILL must not wait for the verbose diagnostic either.
file=kill.status
{ timeout -v -s KILL .1 yes 2>&1
  echo $? >kill.status; } |
  { retry_delay_ file_seen .1 6 || touch kill.missed; }
test -e kill.missed && fail=1
echo 137 > kill.exp || framework_failure_
compare kill.exp kill.status || fail=1

# --kill-after must interrupt a blocked diagnostic for the initial signal.
file=kill-after.status
{ timeout -v -k .1 .1 $SHELL -c 'trap "" TERM; exec yes >&2' 2>&1
  echo $? >kill-after.status; } |
  { retry_delay_ file_seen .1 6 || touch kill-after.missed; }
test -e kill-after.missed && fail=1
compare kill.exp kill-after.status || fail=1

Exit $fail
