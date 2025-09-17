#!/bin/sh
# Make sure all of these programs diagnose read errors

# Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

! cat . >/dev/null 2>&1 || skip_ "Need unreadable directories"

printf '%s' "\
basenc --base32 .
basenc -d --base64 .
cat .
cksum -a blake2b .
cksum -a bsd .
cksum -a crc .
cksum -a crc32b .
cksum -a md5 .
cksum -a sha1 .
cksum -a sha2 -l 224 .
cksum -a sha2 -l 256 .
cksum -a sha2 -l 384 .
cksum -a sha2 -l 512 .
cksum -a sha3 -l 224 .
cksum -a sha3 -l 256 .
cksum -a sha3 -l 384 .
cksum -a sha3 -l 512 .
cksum -a sm3 .
cksum -a sysv .
comm . .
csplit . 1
cut -c1 .
cut -f1 .
date -f .
dd if=.
dircolors .
expand .
factor < .
fmt .
fold .
head -n1 .
head -n-1 .
head -c1 .
head -c-1 .
join . .
nl .
numfmt < .
od .
paste .
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
tac .
tail -n1 .
tail -c1 .
tail -n+1 .
tail -c+1 .
tee < .
tr 1 1 < .
tsort .
unexpand .
uniq .
uniq -c .
wc .
wc -c .
wc -l .
" |
sort -k 1b,1 > all_readers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_readers built_programs > built_readers || framework_failure_

while read reader; do
  eval $reader >/dev/null && { fail=1; echo "$reader: exited with 0" >&2; }
done < built_readers

Exit $fail
