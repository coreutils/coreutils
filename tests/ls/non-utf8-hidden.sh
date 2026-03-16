#!/bin/sh
# Test that files starting with a dot are treated as hidden
# even with invalid UTF-8

# Copyright 2026 Free Software Foundation, Inc.

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

mkdir d || framework_failure_
echo "content" > d/visible || framework_failure_

# Create a hidden file with valid UTF-8
touch d/.hidden_valid || framework_failure_

# Create a hidden file with invalid UTF-8 (without trailing newline)
printf 'content\n' > d/$(printf '.hidden_invalid%s' "$(bad_unicode)") \
  || skip_ 'bad unicode not supported in shell or file system'

for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  test -z "$loc" && continue
  export LC_ALL="$loc"

  # Test 1: Without -a flag, only visible file should appear
  ls -U d > out1 || fail=1
  echo 'visible' > exp1
  compare exp1 out1 || fail=1

  # Test 2: With -a flag, all files including hidden ones should appear
  # The invalid UTF-8 filename will be shown in some escaped form
  ls -a -U d > out2 || fail=1

  # Check that we have at least 5 entries (., .., visible, and 2 hidden files)
  line_count=$(wc -l < out2)
  test "$line_count" -ge 5 || fail=1

  # Test 3: Verify the visible file is still there with -a
  grep -F "visible" out2 > /dev/null || fail=1

  # Test 4: Verify the valid hidden file appears with -a
  grep -F ".hidden_valid" out2 > /dev/null || fail=1

  # Test 5: Verify the invalid hidden file appears with -a
  grep -F ".hidden_invalid" out2 > /dev/null || fail=1
done

Exit $fail
