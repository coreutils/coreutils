#!/bin/sh
# Make sure all of these programs diagnose read errors

# Copyright (C) 2023 Free Software Foundation, Inc.

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

echo "\
basenc --base32 .
basenc -d --base64 .
cat .
cksum -a blake2b .
cksum -a bsd .
cksum -a crc .
cksum -a md5 .
cksum -a sha1 .
cksum -a sha224 .
cksum -a sha256 .
cksum -a sha384 .
cksum -a sha512 .
cksum -a sm3 .
cksum -a sysv .
comm . .
csplit . 1
cut -b1 .
date -f .
dd if=.
dircolors .
expand .
factor < .
fmt .
fold .
head .
join . .
nl .
numfmt < .
od .
paste .
pr .
ptx .
shuf .
sort .
split .
tac .
tail .
tee < .
tr 1 1 < .
tsort .
unexpand .
uniq .
wc .
" |
sort -k 1b,1 > all_readers || framework_failure_

printf '%s\n' $built_programs |
sort -k 1b,1 > built_programs || framework_failure_

join all_readers built_programs > built_readers || framework_failure_

while read reader; do
  eval $reader >/dev/null && { fail=1; echo "$reader: exited with 0" >&2; }
done < built_readers

Exit $fail
