#!/bin/sh
# Test for proper detection of EPIPE with ignored SIGPIPE

# Copyright (C) 2016-2025 Free Software Foundation, Inc.

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
print_ver_ seq
trap_sigpipe_or_skip_

# /bin/sh has an intermittent failure in ignoring SIGPIPE on OpenIndiana 11
# so we require bash as discussed at:
# https://lists.gnu.org/archive/html/coreutils/2020-03/msg00004.html
require_bash_as_SHELL_

# upon EPIPE with signals ignored, 'seq' should exit with an error.
timeout 10 $SHELL -c \
  'trap "" PIPE && { seq inf 2>err; echo $? >code; } | head -n1' >out

# Exit-code must be 1, indicating 'write error'
echo 1 > exp || framework_failure_
compare exp out || fail=1
compare exp code || fail=1

grep '^seq: write error: ' err \
  || { warn_ "seq emitted incorrect error on EPIPE"; \
       cat err;\
       fail=1; }

Exit $fail
