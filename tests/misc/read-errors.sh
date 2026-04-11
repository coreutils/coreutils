#!/bin/sh
# Make sure all of these programs diagnose read errors

# Copyright (C) 2023-2026 Free Software Foundation, Inc.

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
uses_strace_
getlimits_

! cat . >/dev/null 2>&1 || skip_ "Need unreadable directories"

# commands tagged with $count are checked that they output
# the correct number of errors
printf '%s' '\
basenc --base32 .
basenc -d --base64 .
cat . $count
cksum -a blake2b . $count
cksum -a bsd . $count
cksum -a crc . $count
cksum -a crc32b . $count
cksum -a md5 . $count
cksum -a sha1 . $count
cksum -a sha2 -l 224 . $count
cksum -a sha2 -l 256 . $count
cksum -a sha2 -l 384 . $count
cksum -a sha2 -l 512 . $count
cksum -a sha3 -l 224 . $count
cksum -a sha3 -l 256 . $count
cksum -a sha3 -l 384 . $count
cksum -a sha3 -l 512 . $count
cksum -a sm3 . $count
cksum -a sysv . $count
cksum -a md5 /dev/null | sed "s|/dev/null|.|" | cksum --check
comm . .
csplit . 1
cut -b1 . $count
cut -f1 . $count
date -f .
dd if=.
dircolors .
expand . $count
factor < .
fmt . $count
fold . $count
head -n1 . $count
head -n-1 . $count
head -c1 . $count
head -c-1 . $count
join . .
nl . $count
numfmt < .
od . $count
paste . $count
pr .
ptx .
shuf -r .
shuf -n1 .
sort .
split -l1 .
split -b1 .
split -C1 .
split -n1 .
split -nl/1 .
split -nr/1 .
tac . $count
tail -n1 . $count
tail -c1 . $FIXME_count
tail -n+1 . $FIXME_count
tail -c+1 . $FIXME_count
tee < .
tr 1 1 < .
tsort .
unexpand . $count
uniq .
uniq -c .
wc . $count
wc -c . $count
wc -l . $count
' |
sort -k 1b,1 > all_readers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_readers built_programs > built_readers || framework_failure_

count=''
while read reader; do
  eval $reader >/dev/null && { fail=1; echo "$reader: exited with 0" >&2; }
done < built_readers

count='.'  # read '.' twice
while read reader; do
  printf '%s' "$reader" | grep -F '$count' >/dev/null || continue
  eval $reader 2>errs >/dev/null
  test "$?" = 1 ||
    { fail=1; echo "$reader: did not exit with 1" >&2; }
  test "$(grep -F : errs | wc -l)" = 2 ||
    { fail=1; echo "$reader: did not produce 2 errors" >&2; cat errs; }
done < built_readers

expected_failure_status_sort=2

# Ensure read is called, otherwise it's a layering violation.
# Also ensure a read error is diagnosed appropriately.
if strace -o /dev/null -P _ -e '/read,splice' -e fault=all:error=EIO true; then
  while read reader; do
    cmd=$(printf '%s\n' "$reader" | cut -d ' ' -f1) || framework_failure_
    eval "expected=\$expected_failure_status_$cmd"
    test x$expected = x && expected=1
    returns_ $expected \
     strace -f -o /dev/null -P . -e '/read,splice' -e fault=all:error=EIO \
     $SHELL -c "$reader" 2>err || fail=1
    grep -F "$EIO" err >/dev/null || { cat err; fail=1; }
  done < built_readers
fi


Exit $fail
