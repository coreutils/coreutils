#!/bin/sh
# Test that 'date' sets the date properly.

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
print_ver_ date
skip_if_root_  # Just in case
getlimits_
uses_strace_

# Explicitly block in case CAP_SYS_TIME is set

block_settime() {
  settime_calls=clock_settime,clock_settime64,settimeofday,stime
  strace --quiet=all --kill-on-exit -o settime.strace \
   -e inject=$settime_calls:error=EPERM "$@"
}

block_settime true ||
  skip_ 'strace does not support the required options'

# In coreutils-9.11 and coreutils-9.12, TZ="..." date -u MMDDhhmm
# would use the UTC offset from $TZ instead of UTC0.  While we are
# here, check the other methods of setting the date.
date -u -d '2026-01-01 12:00' >exp-out || fail=1
echo "date: cannot set date: $EPERM" >exp-err || framework_failure_
for opt in '' -s --set; do
  if test -z "$opt"; then
    date_str=0101120026
  else
    date_str='2026-01-01 12:00'
  fi
  returns_ 1 block_settime \
   env TZ='America/Los_Angeles' \
   date -u $opt "$date_str" >out 2>err || fail=1
  grep 'EPERM .*INJECTED' settime.strace >/dev/null || {
    cat settime.strace
    test "$fail" = 1 \
     && framework_failure_ 'Possibly incorrect system time set!' \
     || framework_failure_ 'strace failed to intercept "settime" syscall'
  }
  compare exp-out out || fail=1
  compare exp-err err || fail=1
done

Exit $fail
