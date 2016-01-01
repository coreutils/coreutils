#!/bin/sh
# Validate timeout basic operation

# Copyright (C) 2008-2016 Free Software Foundation, Inc.

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
print_ver_ timeout
require_trap_signame_

# no timeout
timeout 10 true || fail=1

# no timeout (suffix check)
timeout 1d true || fail=1

# disabled timeout
timeout 0 true || fail=1

# exit status propagation
timeout 10 sh -c 'exit 2'
test $? = 2 || fail=1

# timeout
timeout .1 sleep 10
test $? = 124 || fail=1

# exit status propagation even on timeout
timeout --preserve-status .1 sleep 10
# exit status should be 128+TERM
test $? = 124 && fail=1

# kill delay. Note once the initial timeout triggers,
# the exit status will be 124 even if the command
# exits on its own accord.
timeout -s0 -k1 .1 sleep 10
test $? = 124 && fail=1

# Ensure 'timeout' is immune to parent's SIGCHLD handler
# Use a subshell and an exec to work around a bug in FreeBSD 5.0 /bin/sh.
(
  trap '' CHLD

  exec timeout 10 true
) || fail=1

# Don't be confused when starting off with a child (Bug#9098).
out=$(sleep .1 & exec timeout .5 sh -c 'sleep 2; echo foo')
status=$?
test "$out" = "" && test $status = 124 || fail=1

Exit $fail
