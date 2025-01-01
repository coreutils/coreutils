#!/bin/sh
# Test env --default-signal=PIPE feature.

# Copyright (C) 2019-2025 Free Software Foundation, Inc.

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

# Paraphrasing https://bugs.gnu.org/34488#8:
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

env_ignore_delay_()
{
  local delay="$1"

  # The first 'env' is just to ensure timeout is not a shell built-in.
  env timeout --verbose --kill-after=.1 --signal=INT $delay \
    env $env_opt sleep 10 > /dev/null 2>outt
  # check only the first two lines from stderr, which are printed by timeout.
  # (operating systems might add more messages, like "killed").
  sed -n '1,2p' outt > out || framework_failure_
  compare exp out
}

# Baseline test - ignore signal handler
# -------------------------------------
# Terminate 'sleep' with SIGINT
# (SIGINT's default action is to terminate a program).
cat <<\EOF >exp || framework_failure_
timeout: sending signal INT to command 'env'
EOF
env_opt='' retry_delay_ env_ignore_delay_ .1 6 || fail=1

# env test - ignore signal handler
# --------------------------------
# Use env to ignore SIGINT - "sleep" should continue running
# after timeout sends SIGINT, and be killed using SIGKILL.
cat <<\EOF >exp || framework_failure_
timeout: sending signal INT to command 'env'
timeout: sending signal KILL to command 'env'
EOF
env_opt='--ignore-signal=INT' retry_delay_ env_ignore_delay_ .1 6 || fail=1
env_opt='--ignore-signal' retry_delay_ env_ignore_delay_ .1 6 || fail=1

# env test --list-signal-handling
env --default-signal --ignore-signal=INT --list-signal-handling true \
  2> err8t || fail=1
sed 's/(.*)/()/' err8t > err8 || framework_failure_
env printf 'INT        (): IGNORE\n' > exp-err8 || framework_failure_
compare exp-err8 err8 || fail=1


Exit $fail
