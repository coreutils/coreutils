#!/bin/sh
# ensure that ls does not stat a symlink in an unusual case

# Copyright (C) 2007-2025 Free Software Foundation, Inc.

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
print_ver_ ls
require_strace_ stat

stats='stat'
# List of other _file name_ stat functions to increase coverage.
other_stats='statx lstat stat64 lstat64 newfstatat fstatat64'
for stat in $other_stats; do
  strace -qe "$stat" true > /dev/null 2>&1 &&
    stats="$stats,$stat"
done

# The system may perform additional stat-like calls before main.
# Furthermore, underlying library functions may also implicitly
# add an extra stat call, e.g. opendir since glibc-2.21-360-g46f894d.
# To avoid counting those, first get a baseline count for running
# ls with one empty directory argument.  Then, compare that with the
# invocation under test.
count_stats() { grep -vE '\+\+\+|ENOSYS|NOTSUP' "$1" | wc -l; }
mkdir d || framework_failure_
strace -q -o log1 -e $stats ls -F --color=always d || fail=1
n_stat1=$(count_stats log1) || framework_failure_

test $n_stat1 = 0 \
  && skip_ 'No stat calls recognized on this platform'


touch x || framework_failure_
chmod a+x x || framework_failure_
ln -s x link-to-x || framework_failure_


# ls from coreutils 6.9 would unnecessarily stat a symlink in an unusual case:
# When not coloring orphan and missing entries, and without ln=target,
# ensure that ls -F (or -d, or -l: i.e., when not dereferencing)
# does not stat a symlink to directory, and does still color that
# symlink and an executable file properly.

LS_COLORS='or=0:mi=0:ex=01;32:ln=01;35' \
  strace -qe $stats -o log2 ls -F --color=always x link-to-x > out.tmp || fail=1
n_stat2=$(count_stats log2) || framework_failure_

# Expect one more stat call,
# which failed with coreutils 6.9 and earlier, which had 2.
test $n_stat1 = $(($n_stat2 - 1)) \
  || { fail=1; head -n30 log*; }

# Check that output is colored, as requested, too.
{
  printf '\033[0m\033[01;35mlink-to-x\033[0m@\n'
  printf '\033[01;32mx\033[0m*\n'
} > exp || fail=1
# Elide info messages strace can send to stdout of the form:
#   [ Process PID=1234 runs in 32 bit mode. ]
sed '/Process PID=/d' out.tmp > out
compare exp out || fail=1

Exit $fail
