#!/bin/sh
# Test --hyperlink processing

# Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

# lookup based on first letter
encode() {
 printf '%s\n' \
  'sp%20ace' 'ques%3ftion' 'back%5cslash' 'encoded%253Fquestion' 'testdir' \
  "$1" |
 sort -k1,1.1 -s | uniq -w1 -d
}

ls_encoded() {
  ef=$(encode "$1")
  echo "$ef" | grep 'dir$' >/dev/null && dir=: || dir=''
  printf '\033]8;;file:///%s\a%s\033]8;;\a%s\n' \
    "$ef" "$1" "$dir"
}

# These could be encoded, so remove from consideration
strip_host_and_path() {
  sed 's|file://.*/|file:///|'
}

mkdir testdir || framework_failure_
(
cd testdir
ls_encoded "testdir" > ../exp.t || framework_failure_
for f in 'back\slash' 'encoded%3Fquestion' 'ques?tion' 'sp ace'; do
  touch "$f" || framework_failure_
  ls_encoded "$f" >> ../exp.t || framework_failure_
done
)
ln -s testdir testdirl || framework_failure_
(cat exp.t && printf '\n' && sed 's/[^\/]testdir/&l/' exp.t) > exp \
  || framework_failure_
ls --hyper testdir testdirl >out.t || fail=1
strip_host_and_path <out.t >out || framework_failure_
compare exp out || fail=1

ln -s '/probably_missing' testlink || framework_failure_
ls -l --hyper testlink > out.t || fail=1
strip_host_and_path <out.t >out || framework_failure_
grep 'file:///probably_missing' out || fail=1

Exit $fail
