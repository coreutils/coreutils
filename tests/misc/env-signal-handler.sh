#!/bin/sh
# Test env --default-signal=PIPE feature.

# Copyright (C) 2019-2020 Free Software Foundation, Inc.

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
print_ver_ env seq test timeout printf
trap_sigpipe_or_skip_

# /bin/sh has an intermittent failure in ignoring SIGPIPE on OpenIndiana 11
# so we require bash as discussed at:
# https://lists.gnu.org/archive/html/coreutils/2020-03/msg00004.html
require_bash_as_SHELL_

# Paraphrasing http://bugs.gnu.org/34488#8:
# POSIX requires that sh started with an inherited ignored SIGPIPE must
# silently ignore all attempts from within the shell to restore SIGPIPE
# handling to child processes of the shell:
#
#    $ (trap '' PIPE; bash -c 'trap - PIPE; seq inf | head -n1')
#    1
#    seq: write error: Broken pipe
#
# With 'env --default-signal=PIPE', the signal handler can be reset to its
# default.

# Baseline Test - default signal handler
# --------------------------------------
# Ensure this results in a "broken pipe" error (the first 'trap'
# sets SIGPIPE to ignore, and the second 'trap' becomes a no-op instead
# of resetting SIGPIPE to its default). Upon a SIGPIPE 'seq' will not be
# terminated, instead its write(2) call will return an error.
(trap '' PIPE; $SHELL -c 'trap - PIPE; seq 999999 2>err1t | head -n1 > out1')

# The exact broken pipe message depends on the operating system, just ensure
# there was a 'write error' message in stderr:
sed 's/^\(seq: write error:\) .*/\1/' err1t > err1 || framework_failure_

printf "1\n" > exp-out || framework_failure_
printf "seq: write error:\n" > exp-err1 || framework_failure_

compare exp-out out1 || framework_failure_
compare exp-err1 err1 || framework_failure_


# env test - default signal handler
# ---------------------------------
# With env resetting the signal handler to its defaults, there should be no
# error message (because the default SIGPIPE action is to terminate the
# 'seq' program):
(trap '' PIPE;
 env --default-signal=PIPE \
    $SHELL -c 'trap - PIPE; seq 999999 2>err2 | head -n1 > out2')

compare exp-out out2 || fail=1
compare /dev/null err2 || fail=1

# env test - default signal handler (3)
# -------------------------------------
# Repeat the previous test, using --default-signal with no signal names,
# i.e., all signals.
(trap '' PIPE;
 env --default-signal \
    $SHELL -c 'trap - PIPE; seq 999999 2>err4 | head -n1 > out4')

compare exp-out out4 || fail=1
compare /dev/null err4 || fail=1

# env test - block signal handler
env --block-signal true || fail=1

# Baseline test - ignore signal handler
# -------------------------------------
# Kill 'sleep' after 1 second with SIGINT - it should terminate (as SIGINT's
# default action is to terminate a program).
# (The first 'env' is just to ensure timeout is not the shell's built-in.)
env timeout --verbose --kill-after=.1 --signal=INT .1 \
    sleep 10 > /dev/null 2>err5

printf "timeout: sending signal INT to command 'sleep'\n" > exp-err5 \
    || framework_failure_

compare exp-err5 err5 || fail=1


# env test - ignore signal handler
# --------------------------------
# Use env to silence (ignore) SIGINT - "seq" should continue running
# after timeout sends SIGINT, and be killed after 1 second using SIGKILL.

cat>exp-err6 <<EOF
timeout: sending signal INT to command 'env'
timeout: sending signal KILL to command 'env'
EOF

env timeout --verbose --kill-after=.1 --signal=INT .1 \
    env --ignore-signal=INT \
    sleep 10 > /dev/null 2>err6t

# check only the first two lines from stderr, which are printed by timeout.
# (operating systems might add more messages, like "killed").
sed -n '1,2p' err6t > err6 || framework_failure_

compare exp-err6 err6 || fail=1


# env test - ignore signal handler (2)
# ------------------------------------
# Repeat the previous test with "--ignore-signals" and no signal names,
# i.e., all signals.

env timeout --verbose --kill-after=.1 --signal=INT .1 \
    env --ignore-signal \
    sleep 10 > /dev/null 2>err7t

# check only the first two lines from stderr, which are printed by timeout.
# (operating systems might add more messages, like "killed").
sed -n '1,2p' err7t > err7 || framework_failure_

compare exp-err6 err7 || fail=1


# env test --list-signal-handling
env --default-signal --ignore-signal=INT --list-signal-handling true \
  2> err8t || fail=1
sed 's/(.*)/()/' err8t > err8 || framework_failure_
env printf 'INT        (): IGNORE\n' > exp-err8
compare exp-err8 err8 || fail=1


Exit $fail
