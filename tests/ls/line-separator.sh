#!/bin/sh
# Ensure ls escapes or hides U+2028 and U+2029 in file names

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
print_ver_ ls printf

{ test -n "$LOCALE_FR_UTF8" && test "$LOCALE_FR_UTF8" != none; } ||
  skip_ "French UTF-8 locale not available"
LC_ALL=$LOCALE_FR_UTF8
export LC_ALL

printable() {
  test "$(env printf '%q' "$(env printf "$1")")" = "$(env printf "$1")"
}

# GLIBC currently considers LINE and PARAGRAPH SEPARATOR as non-printable
# while many other systems do not.  This is not particularly problematic
# on other systems as terminals generally render these horizontally anyway,
# rather than as ambiguous line feeds.  Note all known systems consider
# U+000A (LINE FEED) and U+0085 (NEXT LINE) as non-printable.
{ printable '\u2028' || printable '\u2029'; } &&
  skip_ 'vertical separators are considered printable'

e=$(printf 'a\303\251')
mkdir d || framework_failure_
touch "d/$e" "d/$(printf 'p\342\200\251q')" "d/$(printf 'x\342\200\250y')" ||
  framework_failure_

printf '%s\n' "$e" 'p\342\200\251q' 'x\342\200\250y' > exp ||
  framework_failure_
ls -b d > out || fail=1
compare exp out || fail=1

printf '%s\n' "\"$e\"" '"p\342\200\251q"' '"x\342\200\250y"' > exp ||
  framework_failure_
ls --quoting-style=c d > out || fail=1
compare exp out || fail=1

env printf '%q\n' "$e" "$(printf 'p\342\200\251q')" \
  "$(printf 'x\342\200\250y')"  > exp || framework_failure_
ls --quoting-style=shell-escape d > out || fail=1
compare exp out || fail=1

printf '%s\n' "$e" 'p?q' 'x?y' > exp || framework_failure_
ls -q d > out || fail=1
compare exp out || fail=1

Exit $fail
