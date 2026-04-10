#!/bin/sh
# Make sure all of these programs are responsive to input

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
print_ver_ stdbuf

# stdbuf fails when the absolute top build dir name contains e.g.,
# space, TAB, NL
lf='
'
case $abs_top_builddir in
  *[\\\"\#\$\&\'\`$lf\ \	]*)
    skip_ "unsafe absolute build directory name: $abs_top_builddir";;
esac

stdbuf -oL true || skip_ 'stdbuf not supported'

mkfifo_or_skip_ in
mkfifo_or_skip_ hold

printf '%s' "\
cat
cut -b1
cut -c1
cut -f1
date -f -
expand
#factor TODO
fold
head -n1
head -c1
nl
numfmt
paste
pr
tail -n+1
tail -c+1
tee
tr 1 1
unexpand
uniq
" |
sort -k 1b,1 > all_readers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_readers built_programs > built_readers || framework_failure_

nonempty() { sleep $1; test -s out; }

cleanup_()
{
  kill $writer_pid 2>/dev/null && wait $writer_pid
  kill $reader_pid 2>/dev/null && wait $reader_pid
}

while read reader; do
  rm -f out || framework_failure_
  stdbuf -oL $reader <in >out & reader_pid=$!
  { printf '1\n'; read x <hold || :; } >in & writer_pid=$!
  retry_delay_ nonempty .1 6 ||
    { printf 'no output from: %s\n' "$reader" >&2; fail=1; }
  printf release >hold || framework_failure_
  wait $writer_pid || framework_failure_
  wait $reader_pid || fail=1
done < built_readers


Exit $fail
