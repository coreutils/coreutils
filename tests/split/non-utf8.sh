#!/bin/sh
# Verify that split preserves non-UTF-8 bytes in prefix and suffix.

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
print_ver_ split

echo a > "$(bad_unicode)" \
  || skip_ 'bad unicode not supported in shell or file system'

# Non-UTF-8 bytes in prefix should be preserved, not replaced
# by UTF-8 replacement characters (0xEF 0xBF 0xBD).
prefix="$(bad_unicode)"
printf 'AB' | split -b1 - "$prefix" || fail=1
test -f "$(printf '%saa' $prefix)" || fail=1
test -f "$(printf '%sab' $prefix)" || fail=1

# Non-UTF-8 bytes in --additional-suffix should also be preserved.
suffix="$(bad_unicode)"
printf 'AB' | split -b1 --additional-suffix="$suffix" - q || fail=1
test -f "$(printf 'qaa%s' "$suffix")" || fail=1
test -f "$(printf 'qab%s' "$suffix")" || fail=1

Exit $fail
