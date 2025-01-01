#!/bin/sh
# Validate kill operation

# Copyright (C) 2015-2025 Free Software Foundation, Inc.

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
print_ver_ kill seq

# params required
returns_ 1 env kill || fail=1
returns_ 1 env kill -TERM || fail=1

# Invalid combinations
returns_ 1 env kill -l -l || fail=1
returns_ 1 env kill -l -t || fail=1
returns_ 1 env kill -l -s 1 || fail=1
returns_ 1 env kill -t -s 1 || fail=1

# signal sending
returns_ 1 env kill -0 no_pid || fail=1
env kill -0 $$ || fail=1
env kill -s0 $$ || fail=1
env kill -n0 $$ || fail=1 # bash compat
env kill -CONT $$ || fail=1
env kill -Cont $$ || fail=1
returns_ 1 env kill -cont $$ || fail=1
env kill -0 -1 || fail=1 # to group

# table listing
env kill -l || fail=1
env kill -t || fail=1
env kill -L || fail=1 # bash compat
env kill -t TERM HUP || fail=1

# Verify (multi) name to signal number and vice versa
SIGTERM=$(env kill -l HUP TERM | tail -n1) || fail=1
test $(env kill -l "$SIGTERM") = TERM || fail=1

# Verify we only consider the lower "signal" bits,
# to support ksh which just adds 256 to the signal value
STD_TERM_STATUS=$(expr "$SIGTERM" + 128)
KSH_TERM_STATUS=$(expr "$SIGTERM" + 256)
test $(env kill -l $STD_TERM_STATUS $KSH_TERM_STATUS | uniq) = TERM || fail=1

# Verify invalid signal spec is diagnosed
returns_ 1 env kill -l -1 || fail=1
returns_ 1 env kill -l -1 0 || fail=1
returns_ 1 env kill -l INVALID TERM || fail=1

# Verify all signal numbers can be listed
SIG_LAST_STR=$(env kill -l | tail -n1) || framework_failure_
SIG_LAST_NUM=$(env kill -l -- "$SIG_LAST_STR") || framework_failure_
SIG_SEQ=$(env seq -- 0 "$SIG_LAST_NUM") || framework_failure_
test -n "$SIG_SEQ" || framework_failure_
env kill -l -- $SIG_SEQ || fail=1
env kill -t -- $SIG_SEQ || fail=1

# Verify first signal number listed is 0
test $(env kill -l $(env kill -l | head -n1)) = 0 || fail=1

Exit $fail
