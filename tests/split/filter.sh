#!/bin/sh
# Exercise split's new --filter option.

# Copyright (C) 2011-2026 Free Software Foundation, Inc.

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
print_ver_ split
require_sparse_support_ # for 'truncate --size=$LARGE'
xz --version || skip_ 'xz required'

for total_n_lines in 5 3000 20000; do
  seq $total_n_lines > in || framework_failure_
  for i in 2 51 598; do

    # Don't create too many files/processes.
    # Starting 10k (or even "only" 1500) processes would take a long time,
    # and would provide little added benefit.
    case $i:$total_n_lines in 2:5);; *) continue;; esac

    split -l$i --filter='xz -1 > $FILE.xz' in out- || fail=1
    xz -dc out-* > out || fail=1
    compare in out || fail=1
    rm -f out*
  done
  rm -f in
done

# Show how --elide-empty-files works with --filter:
# split does not run the command (and effectively elides the file)
# only when the output to that command would have been empty.
split -e -n 10 --filter='xz > $FILE.xz' /dev/null || fail=1
returns_ 1 stat x?? 2>/dev/null || fail=1

# Ensure this invalid combination is flagged
returns_ 1 split -n 1/2 --filter='true' /dev/null 2>&1 || fail=1

# Ensure SIGPIPEs sent by the children don't propagate back
# where they would result in a non zero exit from split.
yes | head -n200K | split -b1G --filter='head -c1 >/dev/null' || fail=1

# Ensure that "endless" input is ignored when all filters finish
for mode in '' 'r/'; do
  in_file='-'
  in_cmd='yes'
  if test "$mode" = ''; then
    in_file='zero.in'
    in_cmd='true'
    truncate -s10T "$FILE" || continue
  fi
  for N in 1 2; do
    rm -f x??.n || framework_failure_
    $in_cmd |
     timeout 10 split --filter='head -c1 >$FILE.n' -n $mode$N $in_file || fail=1
    # Also ensure we get appropriate output from each filter
    seq 1 $N | tr '0-9' 1 > stat.exp
    stat -c%s x??.n > stat.out || framework_failure_
    compare stat.exp stat.out || fail=1
  done
done

seq 100 >in &&
echo 1 >exp-xaa &&
sed -n 'n;p' in >exp-xab || framework_failure_
for mode in '' -u; do
  rm -f x?? || framework_failure_
  split $mode -n r/2 \
    --filter='case $FILE in xaa) head -n1 ;; *) cat ;; esac >$FILE' in \
    >out 2>err || fail=1
  compare /dev/null out || fail=1
  compare /dev/null err || fail=1
  compare exp-xaa xaa || fail=1
  compare exp-xab xab || fail=1
done
rm -f x?? || framework_failure_

# Make sure chunks aren't skipped if a filter exits early at the end
# of an input buffer.
# Using 256K which is usually enough to fill a pipe buffer so
# that split(1) has to handle the EPIPE.  FIXME: It may be worth
# adding a function in init.cfg to determine the default pipe size
# or provide a reasonable guess.
for c in a b c d e f g; do
  yes $c | tr -d '\n' | head -c 256k || framework_failure_
done >in
split --bytes 256k ---io-blksize=256k --filter='head -c1 >$FILE' in || fail=1
# coreutils-8.28 to coreutils-9.12 would produce "aceg" here.
test "$(cat x??)" = abcdefg || fail=1
rm -f x?? || framework_failure_

# Make sure that --number chunk sizes aren't affected by a filter exiting early.
{ yes a | tr -d '\n' | head -c 300001 &&
  yes b | tr -d '\n' | head -c 300001 &&
  yes c | tr -d '\n' | head -c 300000; } >in || framework_failure_
split --number 3 ---io-blksize=256k --filter='head -c1 >$FILE' in || fail=1
# coreutils-9.2 to coreutils-9.12 would produce "abb" here.
test "$(cat x??)" = abc || fail=1
rm -f x?? || framework_failure_

# Ensure that "endless" input _is_ processed for unbounded number of filters
for buf in 1000 1000000; do
  returns_ 124 timeout .5 sh -c \
    "yes | split --filter='head -c1 >/dev/null' -b $buf" || fail=1
done

Exit $fail
