#!/bin/sh
# Ensure that cut does not allocate mem for large ranges

# Copyright (C) 2012-2025 Free Software Foundation, Inc.

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
print_ver_ cut
getlimits_

vm=$(get_min_ulimit_v_ returns_ 0 cut -b1 /dev/null) \
  || skip_ 'shell lacks ulimit, or ASAN enabled'

# Ensure we can cut up to our sentinel value.
# Don't use expr to subtract one,
# since UINTMAX_MAX may exceed its maximum value.
CUT_MAX=$(expr $UINTMAX_MAX - 1) || framework_failure_

# From coreutils-8.10 through 8.20, this would make cut try to allocate
# a 256MiB bit vector.
(ulimit -v $vm && cut -b$CUT_MAX- /dev/null > err 2>&1) || fail=1

# Up to and including coreutils-8.21, cut would allocate possibly needed
# memory upfront.  Subsequently extra memory is no longer needed.
(ulimit -v $vm && cut -b1-$CUT_MAX /dev/null >> err 2>&1) || fail=1

# Explicitly disallow values above CUT_MAX
(ulimit -v $vm && returns_ 1 cut -b$UINTMAX_MAX /dev/null 2>/dev/null) ||
  fail=1
(ulimit -v $vm && returns_ 1 cut -b$UINTMAX_OFLOW /dev/null 2>/dev/null) ||
  fail=1

compare /dev/null err || fail=1

Exit $fail
