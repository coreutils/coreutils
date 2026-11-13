#!/bin/sh
# Test the behavior of 'timeout' with a child that uses PTRACE_TRACEME.

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
print_ver_ timeout
require_gcc_shared_

grep '^#define HAVE_PTRACE 1' $CONFIG_HEADER > /dev/null \
  || skip_ 'ptrace is not supported on this system'

# Set up a shared library to use PTRACE_TRACEME upon loading.
cat > k.c <<'EOF' || framework_failure_
#include <sys/ptrace.h>
#include <stddef.h>

static void __attribute__ ((__constructor__))
traceme (void)
{
  ptrace (PTRACE_TRACEME, 0, NULL, NULL);
}
EOF
gcc_shared_ k.c k.so || skip_ 'failed to build ptrace shared lib'

# Make sure we can use it.
timeout --kill-after=1 10 env LD_PRELOAD="$LD_PRELOAD":./k.so \
  grep '^TracerPid:.[1-9][0-9]*$' /proc/self/status \
  > /dev/null \
  || skip_ 'Unable to use PTRACE_TRACEME'

# Test that the signal terminates the program.
# coreutils-9.12 and earlier would print "unknown status" here.
returns_ 124 timeout .1 env LD_PRELOAD="$LD_PRELOAD":./k.so \
  sleep 10 >out 2>err || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# The KILL signal should be sent if the timeout signal is ignored.
# URG is ignored by default, avoiding a race to install SIG_IGN.
# coreutils-9.12 and earlier would exit with a status of 124, but with
# the command left running.
returns_ 137 timeout --foreground --signal=URG --kill-after .1 .1 \
  env LD_PRELOAD="$LD_PRELOAD":./k.so \
  sleep 10 >out 2>err || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# Signals that were not sent by 'timeout' should be delivered to the
# child process.
# coreutils-9.12 and earlier would exit with a status of 1, but with the
# command left running.
returns_ 3 timeout 10 env LD_PRELOAD="$LD_PRELOAD":./k.so \
  "$SHELL" -c 'trap "exit 3" USR1; kill -USR1 "$$"; exit 2' \
  >out 2>err || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

# Make sure a TRAP, when caused by a traced exec, does not
# kill the child process.
returns_ 3 timeout 10 env LD_PRELOAD="$LD_PRELOAD":./k.so \
  "$SHELL" -c 'exec "$SHELL" -c "exit 3"' >out 2>err || fail=1
compare /dev/null out || fail=1
compare /dev/null err || fail=1

Exit $fail
