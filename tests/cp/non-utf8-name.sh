#!/bin/sh
# Ensure cp handles dirs with non-UTF8 names when using recursive copy with dot
# This test covers the case where a directory name contains non-UTF8 bytes

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
print_ver_ cp

non_utf8_dir=$(bad_unicode)
mkdir "$non_utf8_dir" target \
  || skip_ 'bad unicode not supported in shell or file system'

# Create some test files in the non-UTF8 directory
touch "$non_utf8_dir"/file1 "$non_utf8_dir"/file2 || framework_failure_

for loc in C "$LOCALE_FR" "$LOCALE_FR_UTF8"; do
  test -z "$loc" && continue
  export LC_ALL="$loc"

  # Test: copy contents of non-UTF8 directory using /. syntax
  # This should work without panicking or erroring
  cp -r "$non_utf8_dir"/. target || fail=1

  # Verify the files were copied correctly
  rm target/file1 || fail=1
  rm target/file2 || fail=1

  # Verify original files still exist
  test -f "$non_utf8_dir"/file1 || fail=1
  test -f "$non_utf8_dir"/file2 || fail=1
done

Exit $fail
