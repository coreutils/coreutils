#!/bin/sh
# Test 'mktemp' with bad Unicode characters.

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
print_ver_ mktemp

echo a > "$(bad_unicode)" \
  || skip_ 'bad unicode not supported in shell or file system'

for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  test -z "$loc" && continue
  export LC_ALL="$loc"
  # Bad Unicode as a suffix.
  file1=$(mktemp --tmpdir='.' --suffix=$(bad_unicode)) || fail=1
  test -n "$file1" && test -f "$file1" || fail=1
  dir1=$(mktemp --tmpdir='.' -d --suffix=$(bad_unicode)) || fail=1
  test -n "$dir1" &&
  test -d "$dir1" &&
  # Bad Unicode in the argument to --tmpdir.
  file2=$(mktemp --tmpdir="$dir1") &&
  test -n "$file2" && test -f "$file2" &&
  dir2=$(mktemp -d --tmpdir="$dir1") &&
  test -n "$dir2" && test -d "$dir2" &&
  # Bad Unicode in $TMPDIR.
  file3=$(TMPDIR="$dir1" mktemp) &&
  test -n "$file3" && test -f "$file3" &&
  dir3=$(TMPDIR="$dir1" mktemp -d) &&
  test -n "$dir3" && test -d "$dir3" &&
  # Bad Unicode in the argument to -t.
  file4=$(TMPDIR='.' mktemp -t "$(bad_unicode)XXXXXX") &&
  test -n "$file4" && test -f "$file4" &&
  dir4=$(TMPDIR='.' mktemp -d -t "$(bad_unicode)XXXXXX") &&
  test -n "$dir4" && test -d "$dir4" || fail=1
done

Exit $fail
